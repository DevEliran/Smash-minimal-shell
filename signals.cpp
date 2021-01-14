#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
	// send SIGSTP to smash -> smash sends SIGSTOP to the process in the fg
	// if there isn't any process in the fg - ignores signal
	// print "smash: got ctrl-Z" before sending signal
	// print "smash: process <fg-PID> was stopped" after sending signal
    //SmallShell& smash = SmallShell::getInstance();
	//std::cout << "smash: got ctrl-Z" << std::endl;
	//smash.stopFG();


	SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-Z" << std::endl;
    char* cmd = smash.getFgCommand();
    int fg_pid = smash.getForegroundPID();
    int fg_jid = smash.getForegroundJID();
    if(fg_pid != -1){
      smash.getJobs()->addJob(cmd, fg_pid, cmd, true, fg_jid, smash.getForegroundCommandBG());
      if(smash.getIsPipe()){
        if(kill(-fg_pid, SIGSTOP) == -1){
          perror("smash error: kill failed");
        }
      } else{
        if(kill(fg_pid, SIGSTOP) == -1){
          perror("smash error: kill failed");
        }
      }
      std::cout << "smash: process " << fg_pid << " was stopped" << std::endl;
      smash.setForegroundJID(-1);
      smash.setForeGroundPID(-1);
  }
	//if(smash.getForegroundPID() != -1){
      //if(smash.getIsPipe()){
       // if(kill(-(smash.getForegroundPID()), SIGSTOP) == -1){
         // perror("smash error: kill failed");
        //}
      //} else{
        //if(kill(smash.getForegroundPID(), SIGSTOP) == -1){
         // perror("smash error: kill failed");
       // }
     // }
      //smash.getJobs()->addJob(smash.getFgCommand(), smash.getForegroundPID(), smash.getFgCommand(), true, smash.getForegroundJID(), true);
      //std::cout << "smash process " << smash.getForegroundPID() << " was stopped" << std::endl;
      //smash.setForegroundJID(-1);
     // smash.setForeGroundPID(-1);

   // }
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
  // send SIGINT to smash -> smash sends SIGKILL to the process in the fg
  // if there isn't any process in the fg - ignores signal
  // print "smash: got ctrl-C" before sending signal
  // print "smash: process <fg-PID> was killed" after sending signal
  SmallShell& smash = SmallShell::getInstance();
  std::cout << "smash: got ctrl-C" << std::endl;
  int fg_pid = smash.getForegroundPID();
  if(fg_pid != -1){
    if(smash.getIsPipe()){
        if(kill(-fg_pid, SIGKILL) == -1){
          perror("smash error: kill failed");
        }
      } else{
        if(kill(fg_pid, SIGKILL) == -1){
          perror("smash error: kill failed");
        }
      }
      std::cout << "smash: process " << fg_pid << " was killed" << std::endl;
      smash.setForegroundJID(-1);
      smash.setForeGroundPID(-1);
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
  std::cout << "smash: got an alarm" << std::endl;
  SmallShell& smash = SmallShell::getInstance();
  smash.getJobs()->removeFinishedJobs();
  smash.getTimeoutCommands()->removeFinishedAlarms();
  std::list<TimedCmdList::TimedCmd>* timeoutCommands = smash.getTimeoutCommands()->getTimeoutCommands();
  char cmd[COMMAND_MAX_LENGTH];
  int pid = smash.getTimeoutCommands()->getTimeout(cmd);

  if(pid != -1 && waitpid(pid, NULL, WNOHANG) != -1){
    if(kill(-pid, SIGKILL) == -1){
      perror("smash error: kill failed");
    }
    std::cout << "smash: " << cmd << " timed out!" << std::endl;
  }
  smash.getTimeoutCommands()->updateNextAlarm();
  // search which command caused the alarm and send SIGKILL to it
  // print "smash <command> timed out!"
}

