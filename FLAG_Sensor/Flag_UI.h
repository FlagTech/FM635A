#ifndef _FLAG_UI_H
#define _FLAG_UI_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

class FLAG_UI_Object{
  public:
    enum{
      BITMAP,
      TEXT
    };

    uint8_t type;
    uint16_t x;
    uint16_t y;
};

class Flag_UI_Bitmap : public FLAG_UI_Object{
  public:
    uint16_t w;
    uint16_t h;
    uint16_t color;
    const uint8_t* src;
    Flag_UI_Bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color, const uint8_t* src){
      this->type = this->BITMAP;
      this->x = x;
      this->y = y;
      this->w = w;
      this->h = h;
      this->color = color;
      this->src = src;
    };
};

class Flag_UI_Text : public FLAG_UI_Object{
  public:
    uint16_t size;
    uint16_t ftColor;
    uint16_t bgColor;
    const char* src;
    Flag_UI_Text(uint16_t x, uint16_t y, uint16_t size, uint16_t ftColor, uint16_t bgColor, const char* src){
      this->type = this->TEXT;
      this->x = x;
      this->y = y;
      this->size = size;
      this->ftColor = ftColor;
      this->bgColor = bgColor;
      this->src = src;
    };
};

class Flag_UI_Node{
  public:
    FLAG_UI_Object *value;
    Flag_UI_Node *next;
    Flag_UI_Node *prev;
    Flag_UI_Node(){
      this->next = NULL;
      this->prev = NULL;
    };
    Flag_UI_Node(FLAG_UI_Object *value){
      this->next = NULL;
      this->prev = NULL;
      this->value = value;
    };
};

class Flag_UI_Page{
  private:
    Flag_UI_Node *head;
    Flag_UI_Node *current;
  public:
    uint32_t widgetTotal;
    Flag_UI_Page(){
      widgetTotal = 0;
      head = current = NULL;
    };
    
    void addWidget(FLAG_UI_Object *obj){      
      Flag_UI_Node *node = new Flag_UI_Node(obj);
      if(widgetTotal == 0){
        head = current = node;
      }else{
        current->next = node;
        node->prev = current;
        current = node;
      }
      widgetTotal++;
    };

    void show(){
      display.clearDisplay();

      current = head;
      while(current != NULL){
        uint8_t widgetType = current->value->type;
        Flag_UI_Bitmap* img;
        Flag_UI_Text* text;
        switch(widgetType){
          case FLAG_UI_Object::BITMAP:
            img = (Flag_UI_Bitmap*)current->value;
            display.drawBitmap(
              img->x, img->y,
              img->src, 
              img->w, img->h, 
              img->color
            );
            break;
          case FLAG_UI_Object::TEXT:
            text = (Flag_UI_Text*)current->value;  
            display.setTextSize(text->size);
            display.setCursor(text->x, text->y);
            display.setTextColor(text->ftColor, text->bgColor);
            display.println(text->src);
            break;
        }
        if(current->next != NULL)  current = current->next;
        else                       break;
      }
      display.display();
    };
};
#endif