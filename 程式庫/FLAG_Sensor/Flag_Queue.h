#ifndef _FLAG_QUEUE_H
#define _FLAG_QUEUE_H

class Flag_Queue{
  public:
    int32_t buf[10];
    uint32_t in;
    uint32_t out;

    Flag_Queue(){
      memset(buf,0,sizeof(buf));
      in = 0;
      out = 0;
    }

    uint32_t getBufSize(){
      return sizeof(buf) / sizeof(buf[0]); 
    }

    uint8_t push(int32_t data){
      if(isFull()){
        return false;
      }else{
        buf[in] = data;
        in++;
        in %= (sizeof(buf)/sizeof(buf[0]));
        return true;
      }
    }

    void push_force(int32_t data){
      if(isFull()){
        int32_t tmp;
        pop(&tmp);
      }
      buf[in] = data;
      in++;
      in %= (sizeof(buf)/sizeof(buf[0]));
    }

    uint8_t pop(int32_t *data){
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

    float avg(){
      float sum = 0;
      for(int i = 0; i<sizeof(buf)/sizeof(buf[0]); i++){
        sum += buf[i];
      }
      sum /= sizeof(buf)/sizeof(buf[0]);
      return sum;
    }
};

#endif