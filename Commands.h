#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <ctime>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include "signal.h"

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)
#define COMMAND_MAX_LENGTH (80)
#define BUFFER_SIZE (1024)
using namespace std;
extern std::string default_prompt;
extern std::string curr_prompt;


class Command {

 public:
  char* command;
  int pid;
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  Command(const Command& cmd);
  Command& operator=(const Command& cmd);
  const char* getCommand();
  int getPID();
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {

 public:
  BuiltInCommand(const char* cmd_line): Command(cmd_line){};
  virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
 public:
  bool is_pipe;
  ExternalCommand(const char* cmd_line, bool is_pipe): Command(cmd_line), is_pipe(is_pipe){};
  virtual ~ExternalCommand() = default;
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 bool is_pipe;
 int pid_cmd1;
 int pid_cmd2;
 public:
  PipeCommand(const char* cmd_line, bool is_pipe): Command(cmd_line), is_pipe(is_pipe), pid_cmd1(-1), pid_cmd2(-1){}; // might need another member (bool is_pipe)
  virtual ~PipeCommand() = default;
  void execute() override;
  bool getIsPipe(){
    return this->is_pipe;
  }
  void setIsPipe(bool is_pipe){
    this->is_pipe = is_pipe;
  }
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 bool is_pipe;
 public:
  explicit RedirectionCommand(const char* cmd_line, bool is_pipe=false): Command(cmd_line), is_pipe(is_pipe){};
  virtual ~RedirectionCommand() = default;
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangePromptCommand: public BuiltInCommand {
public:
  ChangePromptCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
  virtual ~ChangePromptCommand() = default;
  void execute() override;
};

class LsCommand: public BuiltInCommand {
public:
  LsCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
  virtual ~LsCommand() = default;
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
protected:
    char** lastPWD;
public:
// TODO: Add your data members public:
  ChangeDirCommand(const char* cmd_line, char** plastPwd): BuiltInCommand(cmd_line), lastPWD(plastPwd){};
  virtual ~ChangeDirCommand() = default;
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
public:
  JobsList* jobs;
// TODO: Add your data members public:
  QuitCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~QuitCommand() = default;
  void execute() override;
};



class JobsList {
 public:
  class JobEntry {
  private:

    int job_id;
    int job_pid;
    time_t job_start;
    bool stopped;
    char* command;
    char* print_command;
    bool is_pipe;
    bool is_bg;
   // TODO: Add your data members
   public:
     JobEntry(int job_id, int job_pid, time_t job_start, bool stopped, char* cmd_line, char* cmd_print, bool is_pipe=false, bool is_bg=false): job_id(job_id),
          job_pid(job_pid), job_start(job_start), stopped(stopped), command(cmd_line), print_command(cmd_print), is_pipe(is_pipe), is_bg(is_bg){
            this->command = (char*)malloc(strlen(cmd_line) + 1);
            strcpy(this->command, cmd_line);
            this->print_command = (char*)malloc(strlen(cmd_print) + 1);
            strcpy(this->print_command, cmd_print);
          }
     ~JobEntry(){
       //free(this->command);
       //free(this->print_command);
     }

     void setBg(bool is_bg){
       this->is_bg = is_bg;
     }

     bool getBg(){
       return this->is_bg;
     }

     bool getIsPipe(){
       return this->is_pipe;
     }

     void setJobID(int job_id){
       this->job_id = job_id;
     }

     int getJobID(){
       return this->job_id;
     }

     void setStopped(bool stopped){
       this->stopped = stopped;
     }

     bool getStopped(){
       return this->stopped;
     }

     int getJobPID(){
       return this->job_pid;
     }

     char* getCommand(){
       return this->command;
     }

     char* getPrintCommand(){
       return this->print_command;
     }

     JobEntry(const JobEntry& j){
       this->job_id = j.job_id;
       this->job_pid = j.job_pid;
       this->job_start = j.job_start;
       this->stopped = j.stopped;
       this->command = (char*)malloc(strlen(j.command) + 1);
       strcpy(this->command, j.command);
       this->print_command = (char*)malloc(strlen(j.print_command) + 1);
       strcpy(this->print_command, j.print_command);
       this->is_pipe = j.is_pipe;
       this->is_bg = j.is_bg;
     }

     JobEntry& operator=(const JobEntry& j){
       this->job_id = j.job_id;
       this->job_pid = j.job_pid;
       this->job_start = j.job_start;
       this->stopped = j.stopped;
       this->command = (char*)malloc(strlen(j.command) + 1);
       strcpy(this->command, j.command);
       this->print_command = (char*)malloc(strlen(j.print_command) + 1);
       strcpy(this->print_command, j.print_command);
       this->is_pipe = j.is_pipe;
       this->is_bg = j.is_bg;
       return *this;
     }

     void printJobEntry(){
       double time_diff = difftime(time(nullptr), this->job_start);
       string str(this->print_command);
       if(str[str.find_last_not_of(" \n\r\t\f\v")] != '&' && is_bg){
         this->print_command = const_cast<char*>((string(this->print_command)+"&").c_str());
       }
       std::cout << "[" << this->job_id << "] " << this->command << " : "
            << this->job_pid << " " << time_diff << " secs ";
       if(this->getStopped()){
         std::cout << "(stopped)" << std::endl;
       } else{
         std::cout << std::endl;
       }
     }

     bool operator<(const JobEntry& j){
       return this->job_id < j.job_id;
     }
  };

  std::list<JobEntry>* jobs;
 // TODO: Add your data members
 public:
  JobsList(): jobs(new std::list<JobEntry>){};
  ~JobsList(){
    delete jobs;
  };

  int getJobListSize(){
    return jobs->size();
  }

  void addJob(char* cmd, int job_pid, char* print_cmd, bool isStopped = false, int fg_job_id = -1, bool is_bg=false){
    int job_id;
    if (fg_job_id != -1){
      job_id = fg_job_id;
    } else if(this->jobs->empty()){
      job_id = 1;
    } else {
      this->jobs->sort();
      job_id = this->jobs->back().getJobID() + 1;
    }
    this->jobs->push_back(JobEntry(job_id, job_pid, time(nullptr), isStopped, cmd, print_cmd, false, is_bg));
  };

  JobEntry* getJobById(int jobId){
      for(std::list<JobEntry>::iterator it = this->jobs->begin(); it != this->jobs->end(); ++it){
          if(it->getJobID() == jobId){
            return &(*it);
          }
      }
      return nullptr;
  };

  void printJobsList(){
    this->jobs->sort();
    for(std::list<JobEntry>::iterator it = this->jobs->begin(); it != this->jobs->end(); ++it){
      it->printJobEntry();
    }
  };


  void killAllJobs(){
    for(std::list<JobEntry>::iterator it = this->jobs->begin(); it != this->jobs->end(); ++it){
      std::cout << it->getJobPID() << ": " << it->getCommand() << std::endl;
      if(it->getIsPipe()){
        if(kill(-(it->getJobPID()), SIGKILL) == -1){
          perror("smash error: kill failed");
      }
      }
      if(kill(it->getJobPID(), SIGKILL) == -1){
        perror("smash error: kill failed");
      }
    }
  };

  void removeFinishedJobs(){
      //todo finish
      int curr_job_pid;
      //int jid;
      //int status;
      for(std::list<JobEntry>::iterator it = this->jobs->begin(); it != this->jobs->end();){
          curr_job_pid = it->getJobPID();

          if(waitpid(curr_job_pid, NULL, WNOHANG) == curr_job_pid){
                //if(string(it->getCommand()).find("cp") == 0){

                //}
                it = this->jobs->erase(it);
                if(it == this->jobs->end()){
                  break;
                }
          } else{
            ++it;
          }
          //jobID = it->getJobID();
      }
  };


  void removeJobById(int jobId){
      for(std::list<JobEntry>::iterator it = this->jobs->begin(); it != this->jobs->end(); ++it){
          if(it->getJobID() == jobId){
            this->jobs->erase(it);
            return;
          }
      }
  };


  JobEntry* getCurrJob(int* lastJobId){ // I understood it as curr_job = last_job
    if(this->jobs->empty()){
      return nullptr;
    } else{
      return &(this->jobs->back());
    }
  };

  JobEntry *getLastStoppedJob(){
    for(std::list<JobEntry>::iterator it = this->jobs->end(); it != this->jobs->begin();){
       --it;
      if(it->getStopped()){
        return &(*it);
      }
    }
    return nullptr;
  }; //not sure why required

  JobEntry* getLastJob(){
    if(this->jobs->empty()){
      return nullptr;
    } else{
      return &(this->jobs->back());
    }
  };

  std::list<JobEntry>* getJobs(){
    return this->jobs;
  };

  bool isJobsEmpty(){
    return this->jobs->empty();
  };
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList* jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~JobsCommand() = default;
  void execute() override{
    this->jobs->printJobsList();
  };
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 private:
    JobsList* jobs;
 public:
  KillCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
private:
  JobsList* jobs;
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~ForegroundCommand() = default;
  void execute() override;
  void bringToFg(JobsList::JobEntry* job, char* command, char** args, int args_count);
};

class BackgroundCommand : public BuiltInCommand {
private:
  JobsList* jobs;
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~BackgroundCommand() = default;
  void execute() override;
};


class TimedCmdList{
  public:
    class TimedCmd{
      private:
        int pid;
        time_t timestamp;
        int duration;
        char* command;
      public:
        TimedCmd(int pid, time_t timestamp, int duration, char* cmd_line):
            pid(pid), timestamp(timestamp), duration(duration), command(cmd_line){
              this->command = (char*)malloc(strlen(cmd_line) + 1);
              strcpy(this->command, cmd_line);
            }
        ~TimedCmd(){
          //free(this->command);
        }
        int getPID(){
          return this->pid;
        }
        char* getCommand(){
          return this->command;
        }
        int getDuration(){
          return this->duration;
        }
        time_t getTimestamp(){
          return this->timestamp;
        }
        bool operator<(const TimedCmd& timed_cmd){
          time_t now = time(nullptr);
          if(this->duration - difftime(now, this->timestamp) <
                timed_cmd.duration - difftime(now, timed_cmd.timestamp)){
                  return true;
                }
                return false;
        }
    };
  private:
    std::list<TimedCmd>* timeoutCommands;
  public:
    TimedCmdList(): timeoutCommands(new std::list<TimedCmd>){};
    ~TimedCmdList(){
      delete timeoutCommands;
    }
    void addTimedCommand(int pid, time_t timestamp, int duration,  char* command){
      this->timeoutCommands->push_back(TimedCmd(pid, timestamp, duration, command));
      //updateNextAlarm();
      time_t now = time(nullptr);
      this->timeoutCommands->sort();
      TimedCmd first_element = this->timeoutCommands->front();
      double next_alarm = first_element.getDuration() - difftime(now, first_element.getTimestamp());
      alarm(round(next_alarm));
    }

    std::list<TimedCmd>* getTimeoutCommands(){
      return this->timeoutCommands;
    }

    void updateNextAlarm(){
      time_t now = time(nullptr);
      this->timeoutCommands->sort();
      TimedCmd first_element = this->timeoutCommands->front();
      double next_alarm = first_element.getDuration() - difftime(now, first_element.getTimestamp());
      alarm(round(next_alarm));
    }

    void removeFinishedAlarms(){
      int curr_cmd_pid;
      //int jid;
      //int status;
      for(std::list<TimedCmd>::iterator it = this->timeoutCommands->begin(); it != this->timeoutCommands->end();){
          curr_cmd_pid = it->getPID();

          if(waitpid(curr_cmd_pid, NULL, WNOHANG) == -1){
                it = this->timeoutCommands->erase(it);
                if(it == this->timeoutCommands->end()){
                  break;
                }
          } else{
            ++it;
          }
          //jobID = it->getJobID();
      }
    }

    int getTimeout(char* command){
      time_t now = time(nullptr);
      for(std::list<TimedCmd>::iterator it = timeoutCommands->begin(); it != timeoutCommands->end(); it++){
        double time = it->getDuration() - difftime(now, it->getTimestamp());
        if(round(time) <= 0){
           strcpy(command, it->getCommand());
           int pid = it->getPID();
           this->timeoutCommands->erase(it);
           return pid;
        }
        return -1;
      }
    }

};

class TimeoutCommand: public Command{
  public:
    TimeoutCommand(const char* cmd_line): Command(cmd_line){};
    ~TimeoutCommand() = default;
    void execute() override;
};

class CopyCommand: public Command{
public:
    CopyCommand(const char* cmd_line): Command(cmd_line){};
    ~CopyCommand() = default;
    void execute() override;
};

// TODO: add more classes if needed
// maybe ls, timeout ?

class SmallShell {
 private:
  // TODO: Add your data members
  char** lastPWD;
  bool quit_shell;
  JobsList* jobs;
  int fg_pid;
  int fg_jid;
  char* fg_command;
  bool is_pipe;
  bool is_fg_cmd_bg;
  TimedCmdList* timeoutCommands;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line, bool is_pipe=false);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);

  void setForegroundCommandBG(bool is_bg){
    this->is_fg_cmd_bg = is_bg;
  }

  bool getForegroundCommandBG(){
    return this->is_fg_cmd_bg;
  }
  // TODO: add extra methods as needed
  TimedCmdList* getTimeoutCommands(){
    return this->timeoutCommands;
  }
  void setQuitShell(bool quit){
    this->quit_shell = quit;
  }

  bool getQuitShell(){
    return this->quit_shell;
  }

  int getForegroundPID(){
    return this->fg_pid;
  }

  char* getFgCommand(){
    return this->fg_command;
  }

  void stopFG(){
    if(kill(this->getForegroundPID(), SIGSTOP) == 0){
      jobs->addJob(fg_command, fg_pid, fg_command, true, fg_jid, true);
      kill(fg_pid, SIGSTOP);
      std::cout << "smash: process " << fg_pid << " was stopped" << std::endl;
    }
  }

  void killFG(){
      if(kill(fg_pid,SIGKILL)==0){

      }
  }

  void setForegroundJID(int fg_jid){
    this->fg_jid = fg_jid;
  }

  int getForegroundJID(){
    return this->fg_jid;
  }

  void setForeGroundPID(int fg_pid){
    this->fg_pid = fg_pid;
  }

  void setForegroundCommand(char* command){
    free(this->fg_command);
    this->fg_command = (char*)malloc(strlen(command) + 1);
    strcpy(this->fg_command, command);
  }

  JobsList* getJobs(){
    return this->jobs;
  }

  bool getIsPipe(){
    return this->is_pipe;
  }

  void setIsPipe(bool is_pipe){
    this->is_pipe = is_pipe;
  }
};


#endif //SMASH_COMMAND_H_
