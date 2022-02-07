#ifndef _FM635A_UILITE_H
#define _FM635A_UILITE_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace FM635A_UiLite
{
  enum{
    TEXT,
    IMG,
    BTN,
  };

  class Widget{
    public:
      uint8_t id;
      uint16_t x;
      uint16_t y;
      uint16_t width;
      uint16_t height;
      uint8_t type;
      Widget *next;
  };

  class TextView: public Widget{
    public:
      uint8_t textSize;
      uint8_t inverse;
      String str;
      const uint8_t *img;

      TextView(){
        this->type = TEXT;
        this->textSize = 1;
        this->inverse = 0;
      };
      
      TextView(Widget *w){
        this->id   = w->id;
        this->type = TEXT;
        this->x    = w->x;
        this->y    = w->y;
        this->width  = w->width;
        this->height = w->height;
      }

      TextView(Widget w){
        this->id   = w.id;
        this->type = TEXT;
        this->x    = w.x;
        this->y    = w.y;
        this->width  = w.width;
        this->height = w.height;
      }

      void config(Widget *w){
        this->id   = w->id;
        this->type = TEXT;
        this->x    = w->x;
        this->y    = w->y;
        this->width  = w->width;
        this->height = w->height;
      }

      void config(Widget w){
        this->id   = w.id;
        this->type = TEXT;
        this->x    = w.x;
        this->y    = w.y;
        this->width  = w.width;
        this->height = w.height;
      }

      void setID(uint8_t id){
        this->id = id;
      }

      void setStrImg(const uint8_t *img){
        this->img = img;
      }
      
      void strColorInvertEnable(){
        this->inverse = 1;
      }
      
      void strColorInvertDisable(){
        this->inverse = 0;
      }

      void setTextSize(uint8_t size){
        this->textSize = size;
      }
      void setStr(String str){
        this->str = str;
      }

  };

  class ImageView: public Widget{
    public:
      const uint8_t *img;

      ImageView(){
        this->type = IMG;
      };
      
      ImageView(Widget *w){
        this->id   = w->id;
        this->type = IMG;
        this->x    = w->x;
        this->y    = w->y;
        this->width  = w->width;
        this->height = w->height;
      }

      ImageView(Widget w){
        this->id   = w.id;
        this->type = IMG;
        this->x    = w.x;
        this->y    = w.y;
        this->width  = w.width;
        this->height = w.height;
      }

      void config(Widget *w){
        this->id   = w->id;
        this->type = IMG;
        this->x    = w->x;
        this->y    = w->y;
        this->width  = w->width;
        this->height = w->height;
      }

      void config(Widget w){
        this->id   = w.id;
        this->type = IMG;
        this->x    = w.x;
        this->y    = w.y;
        this->width  = w.width;
        this->height = w.height;
      }

      void setID(uint8_t id){
        this->id = id;
      }

      void setImg(const uint8_t *img){
        this->img = img;
      }
  };

  class Button: public Widget{
    public:
      const uint8_t *img;
      void (*clickCbFunc)(void);

      Button(){
        this->type = BTN;
      };

      Button(Widget *w){
        this->id   = w->id;
        this->type = BTN;
        this->x    = w->x;
        this->y    = w->y;
        this->width  = w->width;
        this->height = w->height;
      }

      Button(Widget w){
        this->id   = w.id;
        this->type = BTN;
        this->x    = w.x;
        this->y    = w.y;
        this->width  = w.width;
        this->height = w.height;
      }
          
      void config(Widget *w){
        this->id   = w->id;
        this->type = BTN;
        this->x    = w->x;
        this->y    = w->y;
        this->width  = w->width;
        this->height = w->height;
      }

      void config(Widget w){
        this->id   = w.id;
        this->type = BTN;
        this->x    = w.x;
        this->y    = w.y;
        this->width  = w.width;
        this->height = w.height;
      }

      void onClick(void (*ptFunc)(void)){
        clickCbFunc = ptFunc;
      }

      void setID(uint8_t id){
        this->id = id;
      }

      void setImg(const uint8_t *img){
        this->img = img;
      }
  };

  class Page{
    public: 
      Adafruit_SSD1306 *display;

      uint8_t id;
      Widget *widgetListHead;
      Widget *widgetListCurrent;

      Page(){
        this->id = 0;
        widgetListCurrent = widgetListHead = NULL;
      }

      Page(uint8_t id){
        this->id = id;
        widgetListCurrent = widgetListHead = NULL;
      }

      void setID(uint8_t id){
        this->id = id;
      }

      void addWidget(Widget* widget){
        if(widgetListHead == NULL){
          widgetListHead = widget;
          widgetListHead->next = NULL;
          widgetListCurrent = widgetListHead;
        }else{
          widgetListCurrent->next = widget;
          widgetListCurrent = widget;
          widgetListCurrent->next = NULL;
        }
      }
      
      void show(){
        widgetListCurrent = widgetListHead;
        this->display->clearDisplay();
        while(1){
          Serial.println(widgetListCurrent->id);
          
          if(widgetListCurrent->type == TEXT){
            this->display->setCursor(widgetListCurrent->x, widgetListCurrent->y);
            this->display->setTextSize(((TextView*)widgetListCurrent)->textSize);
            
            if(((TextView*)widgetListCurrent)->inverse) this->display->setTextColor(BLACK,WHITE);
            else                                        this->display->setTextColor(WHITE,BLACK);
            
            this->display->println(((TextView*)widgetListCurrent)->str);
            this->display->setCursor(0,0);

          }else if(widgetListCurrent->type == IMG){
            this->display->drawBitmap(widgetListCurrent->x, widgetListCurrent->y, ((ImageView*)widgetListCurrent)->img,widgetListCurrent->width,widgetListCurrent->height,WHITE);
          
          }else if(widgetListCurrent->type == BTN){
            this->display->drawBitmap(widgetListCurrent->x, widgetListCurrent->y, ((Button*)widgetListCurrent)->img,widgetListCurrent->width,widgetListCurrent->height,WHITE);
          
          }
          
          if(widgetListCurrent->next == NULL) break;
          else                                widgetListCurrent = widgetListCurrent->next;
        }
        this->display->display();
      }

      void clear(){
        this->display->clearDisplay();
      }
  };
}

#endif