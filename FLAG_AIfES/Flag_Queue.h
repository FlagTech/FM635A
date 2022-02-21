#ifndef _FLAG_QUEUE_H
#define _FLAG_QUEUE_H

class Flag_Queue{
  public:
    uint16_t buf[40];
    uint32_t in;
    uint32_t out;

    Queue(){
      memset(buf,0,sizeof(buf));
      in = 0;
      out = 0;
    }

    uint8_t push(uint16_t data){
      if(isFull()){
        return false;
      }else{
        buf[in] = data;
        in++;
        in %= (sizeof(buf)/sizeof(buf[0]));
        return true;
      }
    }

    void push_force(uint16_t data){
      if(isFull()){
        uint16_t tmp;
        pop(&tmp);
      }
      buf[in] = data;
      in++;
      in %= (sizeof(buf)/sizeof(buf[0]));
    }

    uint8_t pop(uint16_t *data){
      if(isEmpty()){
          return false;
      }else{
        *data = buf[out]; 
        out++;
        out %= (sizeof(buf)/sizeof(buf[0]));
        return true;
      }
    }

    uint8_t isEmpty(){
      if(in == out) return true;
      else          return false;
    }

    uint8_t isFull(){
      if((in+1) % (sizeof(buf)/sizeof(buf[0])) == out) return true;
      else                                             return false;
    }

    uint16_t avg(){
      uint32_t sum = 0;
      for(int i = 0; i<sizeof(buf)/sizeof(buf[0]); i++){
        sum += buf[i];
      }
      sum /= sizeof(buf)/sizeof(buf[0]);
      return (uint16_t) sum;
    }
};

#endif