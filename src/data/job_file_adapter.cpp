
#include <iomanip>

#include "job_file_adapter.hpp"
#include "util/sys/fileutils.hpp"
#include "util/sat_reader.hpp"
#include "util/sys/time_period.hpp"
#include "util/random.hpp"
#include "app/sat/sat_constants.h"
#include "util/sys/terminator.hpp"

void JobFileAdapter::handleNewJob(const FileWatcher::Event& event, Logger& log) {

    if (Terminator::isTerminating()) return;

    log.log(V3_VERB, "New job file event: type %i, name \"%s\"\n", event.type, event.name.c_str());

    nlohmann::json j;
    std::string userFile, jobName;
    int id;
    float userPrio;
    float arrival = Timer::elapsedSeconds();

    {
        auto lock = _job_map_mutex.getLock();
        
        // Attempt to read job file
        std::string eventFile = getJobFilePath(event, NEW);
        if (!std::filesystem::is_regular_file(eventFile)) {
            return; // File does not exist (any more)
        }
        try {
            std::ifstream i(eventFile);
            i >> j;
        } catch (const nlohmann::detail::parse_error& e) {
            log.log(V1_WARN, "Parse error on %s: %s\n", eventFile.c_str(), e.what());
            return;
        }

        // Check and read essential fields from JSON
        if (!j.contains("user") || !j.contains("name") || !j.contains("file")) {
            log.log(V1_WARN, "Job file missing essential field(s). Ignoring this file.\n");
            return;
        }
        std::string user = j["user"].get<std::string>();
        std::string name = j["name"].get<std::string>();
        jobName = user + "." + name + ".json";

        if (_job_name_to_id.count(jobName)) {
            log.log(V1_WARN, "Modification of a file I already parsed! Ignoring.\n");
            return;
        }

        // Get user definition
        userFile = getUserFilePath(user);
        nlohmann::json jUser;
        try {
            std::ifstream i(userFile);
            i >> jUser;
        } catch (const nlohmann::detail::parse_error& e) {
            log.log(V1_WARN, "Unknown user or invalid user definition: %s\n", e.what());
            return;
        }
        if (!jUser.contains("id") || !jUser.contains("priority")) {
            log.log(V1_WARN, "User file %s missing essential field(s). Ignoring job file with this user.\n", userFile.c_str());
            return;
        }
        if (jUser["id"].get<std::string>() != user) {
            log.log(V1_WARN, "User file %s has inconsistent user ID. Ignoring job file with this user.\n", userFile.c_str());
            return;
        }
        
        id = _running_id++;
        userPrio = jUser["priority"].get<float>();
        _job_name_to_id[jobName] = id;
        _job_id_to_image[id] = JobImage(id, jobName, event.name, arrival);

        // Remove original file, move to "pending"
        FileUtils::rm(eventFile);
        std::ofstream o(getJobFilePath(id, PENDING));
        o << std::setw(4) << j << std::endl;
    }

    // Initialize new job
    float time = Timer::elapsedSeconds();
    float priority = userPrio * (j.contains("priority") ? j["priority"].get<float>() : 1.0f);
    if (_params.isNotNull("jjp")) {
        // Jitter job priority
        priority *= 0.99 + 0.01 * Random::rand();
    }
    JobDescription* job = new JobDescription(id, priority, /*incremental=*/false);
    if (j.contains("wallclock-limit")) {
        job->setWallclockLimit(TimePeriod(j["wallclock-limit"].get<std::string>()).get(TimePeriod::Unit::SECONDS));
        log.log(V4_VVER, "Job #%i : wallclock time limit %i secs\n", id, job->getWallclockLimit());
    }
    if (j.contains("cpu-limit")) {
        job->setCpuLimit(TimePeriod(j["cpu-limit"].get<std::string>()).get(TimePeriod::Unit::SECONDS));
        log.log(V4_VVER, "Job #%i : CPU time limit %i CPUs\n", id, job->getCpuLimit());
    }
    job->setArrival(arrival);
    
    // Parse CNF input file
    std::string file = j["file"].get<std::string>();
    SatReader r(file);
    bool success = r.read(*job);
    if (!success) {
        log.log(V1_WARN, "File %s could not be opened - skipping #%i\n", file.c_str(), id);
        return;
    }
    time = Timer::elapsedSeconds() - time;
    log.log(V3_VERB, "Initialized job #%i (%s) in %.3fs: %ld lits w/ separators\n", id, userFile.c_str(), time, job->getFormulaSize());

    // Callback to client: New job arrival.
    _new_job_callback(std::shared_ptr<JobDescription>(job));
}

void JobFileAdapter::handleJobDone(const JobResult& result) {

    if (Terminator::isTerminating()) return;

    auto lock = _job_map_mutex.getLock();

    std::string eventFile = getJobFilePath(result.id, PENDING);
    if (!std::filesystem::is_regular_file(eventFile)) {
        return; // File does not exist (any more)
    }
    std::ifstream i(eventFile);
    nlohmann::json j;
    try {
        i >> j;
    } catch (const nlohmann::detail::parse_error& e) {
        _logger.log(V1_WARN, "Parse error on %s: %s\n", eventFile.c_str(), e.what());
        return;
    }

    // Pack job result into JSON
    j["result"] = { 
        { "resultcode", result.result }, 
        { "resultstring", result.result == RESULT_SAT ? "SAT" : result.result == RESULT_UNSAT ? "UNSAT" : "UNKNOWN" }, 
        { "revision", result.revision }, 
        { "solution", result.solution },
        { "responsetime", Timer::elapsedSeconds() - _job_id_to_image[result.id].arrivalTime }
    };

    // Remove file in "pending", move to "done"
    FileUtils::rm(eventFile);
    std::ofstream o(getJobFilePath(result.id, DONE));
    o << std::setw(4) << j << std::endl;
}

void JobFileAdapter::handleJobResultDeleted(const FileWatcher::Event& event, Logger& log) {

    if (Terminator::isTerminating()) return;

    log.log(V4_VVER, "Result file deletion event: type %i, name \"%s\"\n", event.type, event.name.c_str());

    auto lock = _job_map_mutex.getLock();

    std::string jobName = event.name;
    jobName.erase(std::find(jobName.begin(), jobName.end(), '\0'), jobName.end());
    if (!_job_name_to_id.contains(jobName)) {
        log.log(V1_WARN, "Cannot clean up job \"%s\" : not known\n", jobName.c_str());
        return;
    }

    int id = _job_name_to_id.at(jobName);
    _job_name_to_id.erase(jobName);
    _job_id_to_image.erase(id);
}


std::string JobFileAdapter::getJobFilePath(int id, JobFileAdapter::Status status) {
    return _base_path + (status == NEW ? "/new/" : status == PENDING ? "/pending/" : "/done/") + _job_id_to_image[id].userQualifiedName;
}

std::string JobFileAdapter::getJobFilePath(const FileWatcher::Event& event, JobFileAdapter::Status status) {
    return _base_path + (status == NEW ? "/new/" : status == PENDING ? "/pending/" : "/done/") + event.name;
}

std::string JobFileAdapter::getUserFilePath(const std::string& user) {
    return _base_path + "/../users/" + user + ".json";
}
