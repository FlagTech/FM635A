#ifndef _FLAG_DATAEXPORTER_H
#define _FLAG_DATAEXPORTER_H

class Flag_DataExporter {
  public:
    void dataExport(float *sensor_data, uint32_t data_dim, uint32_t round, uint32_t class_num){
        for(uint32_t z = 0; z < class_num; z++){
            Serial.print("File"); Serial.print(z); Serial.println(".txt");
            for(uint32_t x = 0; x < round; x++){
                for(uint32_t y = 0; y < data_dim; y++){
                Serial.print(sensor_data[z * data_dim * round + x * data_dim + y]);
                if(y < data_dim - 1 ) Serial.print(",");
                }
                Serial.println();
            }
            Serial.println();
        }
    }
};

#endif