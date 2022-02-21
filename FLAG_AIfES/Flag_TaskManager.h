#ifndef _FLAG_TASKMANAGER_H
#define _FLAG_TASKMANAGER_H

#include <Arduino.h>

typedef struct Task_t{
  uint8_t active;
  uint32_t period;
  void (*callback)(void); 
  uint32_t timer;
  struct Task_t *next;
}Flag_Task;

class Flag_TaskManager{
  public:
    enum{
      ONE_SHOT = 0xFFFFFFFF,
    };

    Flag_Task *head;
    Flag_Task *current;
    uint8_t taskTotal;

    Flag_TaskManager(){
      head = NULL;
      current = NULL;
      taskTotal = 0;
      _managerStart=0;
    }

    void add(Flag_Task *task){
      task->next = NULL; //clear next of task
      task->timer = 0;   //reset task timer
      if(taskTotal == 0){ 
        //add first task
        head = task;
        current = head;
      }else{
        current->next = task;
        current = task;
      }
      taskTotal++;
    }
    
    void execute(){
      if(_managerStart == 0){
        _managerStart = 1;
        current = head;
      }
      if(current->active){
        if(current->period == ONE_SHOT){
          (*current->callback)();
          current->active = false;
        }else if(millis() - current->timer >= current->period){
          (*current->callback)();
          current->timer = millis();
        }
      }
      if(current->next == NULL) current = head;
      else                      current = current->next;
    }
  private:
    uint8_t _managerStart;
};

#endif
