#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <dirent.h>
#include <fcntl.h>
#include "Commands.h"

using namespace std;
std::string default_prompt = "smash> ";
std::string curr_prompt = default_prompt;
bool is_first_cd = true;
const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h
void _clear_command(char* cmd, char** args, int args_count){
  for(int i = 0; i < args_count; i++){
      free(args[i]);
    }
    free(cmd);
    return;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line, bool is_pipe) {
  string cmd_s = string(cmd_line);
  if(cmd_s.find("|") != string::npos){
    this->setIsPipe(true);
    return new PipeCommand(cmd_line, this->is_pipe);
  }
  else if(cmd_s.find(">") != string::npos){
    return new RedirectionCommand(cmd_line);
  }
  else if(cmd_s.find("timeout") == 0){
    return new TimeoutCommand(cmd_line);
  }
  else if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (cmd_s.find("cp") == 0){
      return new CopyCommand(cmd_line);
  }
  else if (cmd_s.find("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (cmd_s.find("cd") == 0) {
    return new ChangeDirCommand(cmd_line, this->lastPWD);
  }
  else if (cmd_s.find("chprompt") == 0) {
    return new ChangePromptCommand(cmd_line);
  }
  else if (cmd_s.find("ls") == 0) {
    return new LsCommand(cmd_line);
  }
  else if (cmd_s.find("jobs") == 0){
    return new JobsCommand(cmd_line, this->jobs);
  }
  else if (cmd_s.find("kill") == 0){
    return new KillCommand(cmd_line, this->jobs);
  }
  else if (cmd_s.find("fg") == 0) {
    return new ForegroundCommand(cmd_line, this->jobs);
  }
  else if (cmd_s.find("bg") == 0) {
    return new BackgroundCommand(cmd_line, this->jobs);
  }
  else if (cmd_s.find("quit") == 0) {
    return new QuitCommand(cmd_line, this->jobs);
  }
  else {
    return new ExternalCommand(cmd_line, is_pipe);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  Command* cmd = CreateCommand(cmd_line);
  if(_isBackgroundComamnd(cmd_line)){
    this->setForegroundCommandBG(true);
  }
  if(cmd != nullptr){
    cmd->execute();
  }
  delete cmd;
}

/*
###################################################
############ Command Implementation ###############
###################################################
*/
Command::Command(const char* cmd_line){
    this->command = (char*)malloc(strlen(cmd_line) + 1);
    strcpy(this->command, cmd_line);
}

Command::~Command(){
  free(this->command);
}

Command::Command(const Command& cmd){ // Copy Constructor
  this->command = (char*)malloc(strlen(cmd.command) + 1);
  strcpy(this->command, cmd.command);
  this->pid = cmd.pid;
}

Command& Command::operator=(const Command& cmd){
  this->command = (char*)malloc(strlen(cmd.command) + 1);
  strcpy(this->command, cmd.command);
  this->pid = cmd.pid;
  return *this;
}

const char* Command::getCommand(){
  return this->command;
}

int Command::getPID(){
  return this->pid;
}

/*
###################################################
####### GetCurrDirCommand Implementation ##########
###################################################
*/

void GetCurrDirCommand::execute(){
  char pwd[COMMAND_MAX_LENGTH];
  if (getcwd(pwd, COMMAND_MAX_LENGTH) != NULL){
    std::cout<< pwd << std::endl;
  } else {
    perror("smash error: getcwd failed");
  }
}

/*
###################################################
######## ShowPidCommand Implementation ############
###################################################
*/
void ShowPidCommand::execute(){
  std::cout<<"smash pid is "<< getpid() << std::endl;
}
/*
###################################################
###### ChangePromptCommand Implementation #########
###################################################
*/

void ChangePromptCommand::execute(){
  char* args[COMMAND_ARGS_MAX_LENGTH];
  char* command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  strcpy(command, this->command);
  if (_isBackgroundComamnd(this->command)){
    _removeBackgroundSign(command);
  }
  int args_count = _parseCommandLine(command, args);

  if(args_count == 1){
    curr_prompt = default_prompt;
  }
  else if (args_count > 1){
    curr_prompt = args[1];
    curr_prompt += "> ";
  }
  _clear_command(command, args, args_count);
}
/*
###################################################
####### ChangeDirCommand Implementation ###########
###################################################
*/

void ChangeDirCommand::execute(){
  char* args[COMMAND_ARGS_MAX_LENGTH];
  char* command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  strcpy(command, this->command);
  if (_isBackgroundComamnd(this->command)){
    _removeBackgroundSign(command);
  }
  int args_count = _parseCommandLine(command, args);

  if(args_count > 2){
    std::cerr << "smash error: cd: too many arguments"<< std::endl;
    _clear_command(command, args, args_count);
    return;
  } else if (args_count == 1){
    _clear_command(command, args, args_count);
    return;
  } else if(strcmp(args[1], "-") == 0){
    if(is_first_cd){
      std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
      _clear_command(command, args, args_count);
      return;
    } else {
      char* curr_dir = (char*)malloc(COMMAND_MAX_LENGTH);
      if(getcwd(curr_dir, COMMAND_MAX_LENGTH) == NULL){
        perror("smash error: getcwd failed");
        _clear_command(command, args, args_count);
        return;
      }
      if (chdir(*(this->lastPWD)) == -1){
        perror("smash error: chdir failed");
        _clear_command(command, args, args_count);
        return;
      } else {
        if(!is_first_cd) {
          free(*(this->lastPWD));
        }
        *(this->lastPWD) = (char*)malloc(strlen(curr_dir) + 1);
        strcpy(*(this->lastPWD), curr_dir);
        free(curr_dir);
      }
    }
    _clear_command(command, args, args_count);
    return;
  }
  char* old_dir = (char*)malloc(COMMAND_MAX_LENGTH);
  if(getcwd(old_dir, COMMAND_MAX_LENGTH) == NULL){
    perror("smash error: getcwd failed");
    _clear_command(command, args, args_count);
    return;
  }
  if(chdir(args[1]) == -1){
    perror("smash error: chdir failed");
    _clear_command(command, args, args_count);
    free(old_dir);
    return;
  } else {
    if(!is_first_cd){
      free(*(this->lastPWD));
    }
    *(this->lastPWD) = (char*)malloc(strlen(old_dir) + 1);
    strcpy(*(this->lastPWD), old_dir);
  }
  if(is_first_cd){
    is_first_cd = false;
  }
  _clear_command(command, args, args_count);
  free(old_dir);
}

/*
###################################################
####### LsCommand Implementation ###########
###################################################
*/

void LsCommand::execute(){
  struct dirent **files;
  int i, n;
  n = scandir(".", &files, 0, alphasort);
  if (n < 0){
    perror("smash error: scandir failed");
  } else {
  for (i = 2; i < n; i++) {
    std::cout << files[i]->d_name<< std::endl;
    free(files[i]);
  }
  }
  free(files);
}

/*
###################################################
############ Kill Implementation ##################
###################################################
*/

void KillCommand::execute(){
  char* args[COMMAND_ARGS_MAX_LENGTH];
  char* cmd = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  strcpy(cmd, this->command);
  if (_isBackgroundComamnd(this->command)){
    _removeBackgroundSign(cmd);
  }
  int args_count = _parseCommandLine(cmd, args);

  if(args_count != 3){
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    _clear_command(cmd, args, args_count);
    return;
  }

  int sig_num, job_id;
  try{
    sig_num = stoi(args[1]);
    job_id = stoi(args[2]);
  } catch(std::invalid_argument&){
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    _clear_command(cmd, args, args_count);
    return;
  }
  if(sig_num > 0){
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    _clear_command(cmd, args, args_count);
    return;
  }

  JobsList::JobEntry* job = this->jobs->getJobById(job_id);
  if(job == nullptr){
    std::cerr << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
    _clear_command(cmd, args, args_count);
    return;
  }
  if(kill(-job->getJobPID(), abs(sig_num)) == -1){
    perror("smash error: kill failed");
    _clear_command(cmd, args, args_count);
    return;
  }
  std::cout << "signal number " << abs(sig_num) << " was sent to pid " << job->getJobID() << std::endl;
  //if(abs(sig_num) == SIGSTOP){
    //job->setStopped(true);
 // }
  //if(abs(sig_num) == SIGCONT){
   // job->setStopped(false);
  //}
  _clear_command(cmd, args, args_count);
}


/*
###################################################
####### ForegroundCommand Implementation ##########
###################################################
*/

void ForegroundCommand::bringToFg(JobsList::JobEntry* job, char* command, char** args, int args_count){
    if(job == nullptr){
      return;
    }
    SmallShell& smash = SmallShell::getInstance();
    char* job_cmd = job->getCommand();
    int job_id = job->getJobID();
    int job_pid = job->getJobPID();
    smash.setForegroundJID(job_id);
    smash.setForegroundCommand(job_cmd);
    smash.setForeGroundPID(job_pid);
    smash.setForegroundCommandBG(job->getBg());
    if(job->getIsPipe()){
      smash.setIsPipe(true);
    }
    std::cout << job_cmd << " : " << job_pid << std::endl;

    if(job->getStopped()){
      if(job->getIsPipe()){
        if(kill(-job_pid, SIGCONT) == -1){
        perror("smash error: kill failed");
        _clear_command(command, args, args_count);
        }
      } else{
        if(kill(job_pid, SIGCONT) == -1){
          perror("smash error: kill failed");
          _clear_command(command, args, args_count);
        }
        job->setStopped(false);
      }
    }
    this->jobs->removeJobById(job_id);
    if(waitpid(job_pid, NULL,0 | WUNTRACED) == -1){
      perror("smash erorr: waitpid failed");
      _clear_command(command, args, args_count);
    }

    smash.setForeGroundPID(-1);
    smash.setForegroundJID(-1);
}

void ForegroundCommand::execute(){
  char* args[COMMAND_ARGS_MAX_LENGTH];
  char* command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  strcpy(command, this->command);
  if (_isBackgroundComamnd(this->command)){
    _removeBackgroundSign(command);
  }
  int args_count = _parseCommandLine(command, args);

  if(args_count != 1 && args_count != 2){
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
    _clear_command(command, args, args_count);
  }

  if(args_count == 1){ //bring last job to the foreground
    JobsList::JobEntry* last_job = this->jobs->getLastJob();
    if(last_job  == nullptr){
      std::cerr << "smash error: fg: jobs list is empty" << std::endl;
      _clear_command(command, args, args_count);
      return;
    }
    bringToFg(last_job, command, args, args_count);

  }

  if(args_count == 2){
    int job_id;
    try{
      job_id = stoi(args[1]);
    }
    catch(std::invalid_argument&){
      std::cerr << "smash error: fg: invalid arguments" << std::endl;
      _clear_command(command, args, args_count);
      return;
    }

    JobsList::JobEntry* job = this->jobs->getJobById(job_id);
    if(job == nullptr){
      std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << std::endl;
      _clear_command(command, args, args_count);
      return;
    }
    bringToFg(job, command, args, args_count);
  }
  _clear_command(command, args, args_count);
}

/*
###################################################
####### BackgroundCommand Implementation ##########
###################################################
*/

void BackgroundCommand::execute(){
  char* args[COMMAND_ARGS_MAX_LENGTH];
  char* command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  strcpy(command, this->command);
  if (_isBackgroundComamnd(this->command)){
    _removeBackgroundSign(command);
  }
  int args_count = _parseCommandLine(command, args);

  if(args_count != 1 && args_count != 2){
    std::cerr << "smash error: bg: invalid arguments" << std::endl;
    _clear_command(command, args, args_count);
    return;
  }

  if(args_count == 1){ //continue last stopped job in bg
    JobsList::JobEntry* last_stopped_job = this->jobs->getLastStoppedJob();
    if(last_stopped_job == nullptr){
      std::cerr << "smash error: bg: there is no stopped jobs to resume" << std::endl;
      _clear_command(command, args, args_count);
      return;
    }
    std::cout << last_stopped_job->getCommand()  << " : " << last_stopped_job->getJobPID() << std::endl;
    if(last_stopped_job->getIsPipe()){
      if(kill(-(last_stopped_job->getJobPID()), SIGCONT) == -1){
        perror("smash error: kill failed");
        _clear_command(command, args, args_count);
        return;
      }
      last_stopped_job->setStopped(false);
      return;
    }
    if(kill(last_stopped_job->getJobPID(), SIGCONT) == -1){
      perror("smash error: kill failed");
      _clear_command(command, args, args_count);
      return;
    }
    last_stopped_job->setStopped(false);
    return;
  }

  if(args_count == 2){
    int job_id;
    try{
      job_id = stoi(args[1]);
    }
    catch(std::invalid_argument&){
      std::cerr << "smash error: bg: invalid arguments" << std::endl;
      _clear_command(command, args, args_count);
      return;
    }
    JobsList::JobEntry* job = this->jobs->getJobById(job_id);
    if(job == nullptr){
      std::cerr << "smash error: bg: job-id " << job_id << " does not exist" << std::endl;
      _clear_command(command, args, args_count);
      return;
    }
    if(!job->getStopped()){
      std::cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << std::endl;
      _clear_command(command, args, args_count);
      return;
    } else{
      std::cout << job->getCommand() << " : " << job->getJobPID() << std::endl;
      if(job->getIsPipe()){
        if(kill(-(job->getJobPID()), SIGCONT) == -1){
          perror("smash error: kill failed");
          _clear_command(command, args, args_count);
          return;
        }
        job->setStopped(false);
        return;
      }
      if(kill(job->getJobPID(), SIGCONT) == -1){
        perror("smash error: kill failed");
        _clear_command(command, args, args_count);
        return;
      }
      job->setStopped(false);
      return;
    }
  }
  _clear_command(command, args, args_count);
}


/*
###################################################
########## QuitCommand Implementation #############
###################################################
*/

void QuitCommand::execute(){
  char* args[COMMAND_ARGS_MAX_LENGTH];
  char* command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  strcpy(command, this->command);
  if (_isBackgroundComamnd(this->command)){
    _removeBackgroundSign(command);
  }
  int args_count = _parseCommandLine(command, args);

  if(args_count > 1){
    for(int i = 0; i < args_count; i++){
      if(strcmp(args[i], "kill") == 0){
        std::cout << "smash: sending SIGKILL signal to " << this->jobs->getJobListSize() << " jobs:"<< std::endl;
        this->jobs->killAllJobs();
      }
    }
  }
  _clear_command(command, args, args_count);
  SmallShell& smash = SmallShell::getInstance();
  smash.getJobs()->removeFinishedJobs();
  smash.setQuitShell(true);
}

/*
###################################################
######## ExternalCommand Implementation ###########
###################################################
*/

void ExternalCommand::execute(){
  char* print_command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  char* command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  strcpy(command, this->command);
  strcpy(print_command, this->command);
  if (_isBackgroundComamnd(this->command)){
    _removeBackgroundSign(command);
  }

  pid_t pid = fork();
  if(pid == -1){
    perror("smash error: fork failed");
  }
  if(pid == 0){ //child process
    if(!(this->is_pipe)){
      if(setpgrp() == -1){
        perror("smash error: setpgrp failed");
      }
    }
    char* args[] = {(char*)"/bin/bash", (char*)"-c", command, NULL};
    execv(args[0], args);
    perror("smash error: execv failed");
  } else{ //father process
    this->pid = pid;
    SmallShell& smash = SmallShell::getInstance();
    if(_isBackgroundComamnd(this->command)){
      smash.getJobs()->addJob(this->command, pid, this->command, false, -1, true);
    } else{
      smash.setForeGroundPID(pid);
      smash.setForegroundJID(-1);
      smash.setForegroundCommand(command);
      waitpid(pid, NULL, 0 | WUNTRACED);
      smash.setForeGroundPID(-1);
    }
  }
  free(command);
}

/*
###################################################
########## PipeCommand Implementation #############
###################################################
*/
void PipeCommand::execute(){
  char* args1[COMMAND_ARGS_MAX_LENGTH];
  char* args2[COMMAND_ARGS_MAX_LENGTH];
  char* command1 = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  char* command2 = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  char* command1_print = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  char* command2_print = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  int cmd_sep_out = string(this->command).find("|");
  int cmd_sep_err = string(this->command).find("|&");

  strcpy(command1, string(this->command).substr(0, cmd_sep_out).c_str());
  if(cmd_sep_err != string::npos){
    strcpy(command2, string(this->command).substr(cmd_sep_err + 2).c_str());
  } else{
    strcpy(command2, string(this->command).substr(cmd_sep_out + 1).c_str());
  }

  strcpy(command1_print, string(this->command).substr(0, cmd_sep_out).c_str()); //was -1
  if(cmd_sep_err != string::npos){
    strcpy(command2_print, string(this->command).substr(cmd_sep_err + 1).c_str());
  } else{
    strcpy(command2_print, string(this->command).substr(cmd_sep_out + 1).c_str());
  }

  int args_count1 = _parseCommandLine(command1, args1);
  int args_count2 = _parseCommandLine(command2, args2);

  if (_isBackgroundComamnd(command1)){
    _removeBackgroundSign(command1);
  }
  bool is_bg = false;
  if (_isBackgroundComamnd(command2)){
    _removeBackgroundSign(command2);
    is_bg = true;
  }

  SmallShell& smash = SmallShell::getInstance();
  pid_t smash_pid = getpid();

  int stream = cmd_sep_err != string::npos ? STDERR_FILENO: STDOUT_FILENO;

    int first_fork = fork();
    if(first_fork == -1){
     perror("smash error: fork failed");
     return;
    }
    if(first_fork == 0){
        if(!is_pipe) setpgrp();
        int fd[2];
        if (pipe(fd) == -1){
          perror("smash error: pipe failed");
          return;
        }
        int child_p1=fork();
        if(child_p1==-1){
          perror("smash error: fork failed");
          return;
        }
        if(child_p1 == 0){
            dup2(fd[1], stream);
            close(fd[0]);
            close(fd[1]);
            Command* cmd1 = smash.CreateCommand(command1,true);
            if(strcmp(args1[0],"showpid")==0){
              std::cout << "smash pid is " << smash_pid << endl;
            } else{
              cmd1->execute();
            }
            delete cmd1;
            smash.setQuitShell(true);
            _clear_command(command1, args1, args_count1);
            _clear_command(command2, args2, args_count2);
            return;
        }
        int child_p2=fork();
        if(child_p2 == -1){
          perror("smash error: fork failed");
          return;
        }
        if(child_p2 == 0){
            dup2(fd[0],0);
            close(fd[0]);
            close(fd[1]);
            Command* cmd2=smash.CreateCommand(command2,true);
            if(strcmp(args2[0],"showpid")==0){
              std::cout << "smash pid is " << smash_pid << endl;
            }
            else{
              cmd2->execute();
            }
            delete cmd2;
            smash.setQuitShell(true);
            _clear_command(command1, args1, args_count1);
            _clear_command(command2, args2, args_count2);
            return;
        }
        if(close(fd[0]) == -1 || close(fd[1]) == -1){
          perror("smash error: close failed");
          _clear_command(command1, args1, args_count1);
          _clear_command(command2, args2, args_count2);
          return;
        }
        waitpid(child_p1,NULL,0);
        waitpid(child_p2,NULL,0);

        smash.setQuitShell(true);

        _clear_command(command1, args1, args_count1);
        _clear_command(command2, args2, args_count2);
        return;
    } else{
        this->pid=first_fork;
        if(!is_bg){
            smash.setForeGroundPID(pid);
            smash.setForegroundCommand(this->command);
            smash.setForegroundJID(-1);
            waitpid(pid,NULL,WUNTRACED);
            smash.setForeGroundPID(-1);
        }
        else{
            smash.getJobs()->addJob(this->command, this->pid,this->command, false, -1 ,true);
        }
    }
    _clear_command(command1, args1, args_count1);
    _clear_command(command2, args2, args_count2);
  }
/*
###################################################
####### RedirectionCommand Implementation #########
###################################################
*/
void RedirectionCommand::execute(){

  char* args[COMMAND_ARGS_MAX_LENGTH];
  char* command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  char* command_print = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);

  char* file_path = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);

  int cmd_sep_write = string(this->command).find(">");
  int cmd_sep_append = string(this->command).find(">>");

  strcpy(command, string(this->command).substr(0, cmd_sep_write).c_str());
  if(cmd_sep_append != string::npos){
    strcpy(file_path, string(this->command).substr(cmd_sep_append + 2).c_str());
  } else{
    strcpy(file_path, string(this->command).substr(cmd_sep_write + 1).c_str());
  }

  strcpy(command_print, string(this->command).substr(0, cmd_sep_write).c_str());

  int args_count = _parseCommandLine(command, args);

  char* args_path[COMMAND_ARGS_MAX_LENGTH];

  int args_count_path = _parseCommandLine(file_path, args_path);

  bool is_bg = false;
  if(_isBackgroundComamnd(command)){
    _removeBackgroundSign(command);
  }
  if(_isBackgroundComamnd(file_path)){
    is_bg = true;
    _removeBackgroundSign(file_path);
  }

  SmallShell& smash=SmallShell::getInstance();

  int stdout = dup(STDOUT_FILENO);
  if(stdout == -1){
    perror("smash error: dup failed");
    _clear_command(command, args, args_count);
    _clear_command(file_path, args_path, args_count_path);
    return;
  }
  if(close(STDOUT_FILENO) == -1){
    perror("smash error: close failed");
    dup2(stdout, STDOUT_FILENO);
    close(stdout);
    _clear_command(command, args, args_count);
    _clear_command(file_path, args_path, args_count_path);
    return;
  }

  int file;
  if(cmd_sep_append != string::npos){
    file = open(args_path[0], O_CREAT | O_RDWR | O_APPEND , 0666);
  } else{
    file = open(args_path[0], O_CREAT | O_RDWR | O_TRUNC , 0666);
  }
  if(file == -1){
      perror("smash error: open failed");
      dup2(stdout,STDOUT_FILENO);
      close(stdout);
      _clear_command(command, args, args_count);
      _clear_command(file_path, args_path, args_count_path);
      return;
    }

  Command* cmd;
  if(this->is_pipe){
    cmd = smash.CreateCommand(command, true);
  } else{
    cmd = smash.CreateCommand(command);
  }
  int smash_pid = getpid();
  std::string command_string = string(command);
  if(command_string.find("chprompt") == 0 || command_string.find("pwd") == 0 ||
      command_string.find("showpid") == 0 || command_string.find("cd") == 0 ||
      command_string.find("jobs") == 0 || command_string.find("kill") == 0 ||
      command_string.find("fg") == 0 || command_string.find("bg") == 0 ||
      command_string.find("quit") == 0){
      cmd->execute();
      this->pid = cmd->getPID();
      delete cmd;
      close(file);
    } else{
        int pid=fork();
        if(pid==-1){
          perror("smash error: fork failed");
        }
        if(pid==0){
          if(!(this->is_pipe)){
            setpgrp();
          }
          if(strcmp(args[0],"showpid")==0){
            std::cout << "smash pid is " << smash_pid << endl;
          } else{
            cmd->execute();
          }
          delete cmd;
          smash.setQuitShell(true);
          close(file);
          dup2(stdout,STDOUT_FILENO);
          close(stdout);
          _clear_command(command, args, args_count);
          _clear_command(file_path, args_path, args_count_path);
          return;
         } else{
             close(file);
             this->pid=pid;
          }
        }
        if(!is_bg){
          smash.setForeGroundPID(pid);
          smash.setForegroundCommand(this->command);
          waitpid(pid,NULL,0 | WUNTRACED);
          smash.setForeGroundPID(-1);
        }
        else{
          smash.getJobs()->addJob(this->command, this->pid, this->command, false, -1, true);
        }
        dup2(stdout,STDOUT_FILENO);
        close(stdout);
        _clear_command(command, args, args_count);
        _clear_command(file_path, args_path, args_count_path);
    }


/*
###################################################
########## TimeoutCommand Implementation ##########
###################################################
*/

void TimeoutCommand::execute(){
  char* args[COMMAND_ARGS_MAX_LENGTH];
  char* command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  char* command_print = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);

  strcpy(command_print, this->command);
  strcpy(command, this->command);
  bool is_bg;
  if(_isBackgroundComamnd(command)){
    _removeBackgroundSign(command);
    is_bg = true;
  }

  int args_count = _parseCommandLine(command, args);

  if(args_count < 3){
    std::cerr << "smash error: timeout: invalid arguments" << std::endl;
    _clear_command(command, args, args_count);
    return;
  }
  int duration;
  try{
    duration = stoi(args[1]);
  } catch(std::invalid_argument&){
    std::cerr << "smash error: timeout: invalid arguments" << std::endl;
    _clear_command(command, args, args_count);
    return;
  }

  if(duration < 0){
    std::cerr << "smash error: timeout: invalid arguments" << std::endl;
    _clear_command(command, args, args_count);
    return;
  }

  string cmd;
  for(int i = 2; i < args_count; i ++){
    cmd += args[i];
    if(i != args_count - 1){
      cmd += " ";
    }
  }

  SmallShell& smash = SmallShell::getInstance();
  Command* c = smash.CreateCommand(cmd.c_str());
  //c->execute();
  //this->pid = c->pid;
  //delete c;
  std::string command_string = string(command);
  if(command_string.find("chprompt") == 0 || command_string.find("pwd") == 0 ||
      command_string.find("showpid") == 0 || command_string.find("cd") == 0 ||
      command_string.find("jobs") == 0 || command_string.find("kill") == 0 ||
      command_string.find("fg") == 0 || command_string.find("bg") == 0 ||
      command_string.find("quit") == 0){
        c->execute();
        this->pid = c->getPID();
        delete c;
      }
  else{
    int fork_pid = fork();
    if(fork_pid == -1){
      perror("smash error: fork failed");
      return;
    }
    if(fork_pid == 0){
      setpgrp();
      c->execute();
      delete c;
      smash.setQuitShell(true);
      _clear_command(command, args, args_count);
      return;
    } else{
      this->pid = fork_pid;
    }
  }

  smash.getTimeoutCommands()->addTimedCommand(this->pid, time(nullptr), duration, command_print);

  if(is_bg){
    smash.getJobs()->addJob(this->command, this->pid, this->command, false, -1, true);
  } else{
    smash.setForeGroundPID(pid);
    smash.setForegroundCommand(this->command);
    waitpid(this->pid, NULL, 0 | WUNTRACED);
    smash.setForeGroundPID(-1);
  }
  _clear_command(command, args, args_count);
}

/*
###################################################
############# CopyCommand Implementation ##########
###################################################
*/
void CopyCommand::execute() {
    char* args[COMMAND_ARGS_MAX_LENGTH];
    char* command = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
    char* buffer = (char*)malloc(BUFFER_SIZE);
    //char buffer[1024]
    strcpy(command, this->command);
    bool is_bg = _isBackgroundComamnd(this->command);
    if (is_bg){
        _removeBackgroundSign(command);
    }
    int args_count = _parseCommandLine(command, args);
    int src_file, dst_file, num_of_chars;
    size_t buffer_size = 1024;
    SmallShell &smash = SmallShell::getInstance();


    if(args_count != 3){
        std::cerr << "smash error: cp: invalid amount of arguments"<< std::endl;
        _clear_command(command, args, args_count);
    }
    int pid=fork();
    if(pid==-1){
        perror("smash error: fork failed");
    }
    else if(pid==0){//child
        setpgrp();
        if((src_file = open(args[1], O_RDONLY, 0666)) < 0){
            perror("smash error: open failed");
        }
        if((dst_file = open(args[2], O_WRONLY | O_CREAT | O_TRUNC , 0666)) < 0){
            perror("smash error: open failed");
        }
        while((num_of_chars = read(src_file, buffer, buffer_size)) > 0){
           if(write(dst_file, buffer, num_of_chars) != num_of_chars){
                   perror("smash error: write failed");
               }
        }
        if(num_of_chars == -1){
            perror("smash error: write failed");
        }
        if( close(src_file) == -1 || close(dst_file) == -1 ){
            perror("smash error: close failed");
        }
        std::cout << "smash: " << args[1] << " was copied to "<< args[2] << std::endl;
        smash.setQuitShell(true);
        _clear_command(command, args, args_count);
        return;
    } else { //parent
        this->pid = pid;
    }

    if(!is_bg){
        smash.setForeGroundPID(pid);
        smash.setForegroundCommand(this->command);
        waitpid(pid,NULL,0 | WUNTRACED);
        smash.setForeGroundPID(-1);
    }
    else{
        smash.getJobs()->addJob(this->command, this->pid, this->command, false, -1, true);
    }

    free(buffer);
    _clear_command(command, args, args_count);
}

/*
###################################################
####### SmallShell Implementation ###########
###################################################
*/
SmallShell::SmallShell() {
  this->lastPWD = (char**)malloc(sizeof(char*));
  *(this->lastPWD) = nullptr;
  this->quit_shell = false;
  this->jobs = new JobsList;
  this->fg_command = nullptr;
  this->is_pipe = false;
  this->fg_jid = -1;
  this->fg_pid = -1;
  this->timeoutCommands = new TimedCmdList;
  this->is_fg_cmd_bg = false;
}

SmallShell::~SmallShell() {
  free(*lastPWD);
  free(lastPWD);
  free(jobs);
  free(timeoutCommands);
}

