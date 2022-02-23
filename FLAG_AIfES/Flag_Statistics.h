#ifndef _FLAG_STATISTICS_H
#define _FLAG_STATISTICS_H

#include <math.h>

class Flag_Statistics{
  public:
    //平均值算法
    float mean(void *buf, uint32_t len){
      float sum = 0;

      for(int i = 0; i<len; i++){
        sum+=((float*) buf)[i];
      }
      sum /= len;
      return sum;
    }

    //標準差算法
    float sd(void *buf, uint32_t len){
      float sum = 0;
      float Mean = mean(buf, len);

      for(int i = 0; i<len; i++){
        sum += sq( ((float*)buf)[i] - Mean );
      }
            
      sum /= len;
      return sqrt(sum);
    }

    //找出最大絕對值
    float maxAbs(float *data, uint32_t len){
      float maximum = 0;
      for(uint32_t i = 0; i < len; i++ ){
        if(i == 0){
          maximum = abs(data[i]);
        }else{
          if(abs(data[i]) > maximum){
            maximum = abs(data[i]);
          }
        }
      }
      return maximum;
    }
};
#endif