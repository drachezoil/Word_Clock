/**
 * SyncArduinoClock. 
 *
 * portIndex must be set to the port connected to the Arduino
 * 
 * The current time is sent in response to request message from Arduino 
 * or by clicking the display window 
 *
 * The time message is 11 ASCII text characters; a header (the letter 'T')
 * followed by the ten digit system time (unix time)
 */

import processing.serial.*;
import java.util.Date;
import java.util.Calendar;
import java.util.GregorianCalendar;
import controlP5.*;
import java.util.*;

public static final short portIndex = 1;  // select the com port, 0 is the first port
public static final String TIME_HEADER = "T"; //header for arduino serial time message 
public static final char TIME_REQUEST = 7;  // ASCII bell character 
public static final char LF = 10;     // ASCII linefeed
public static final char CR = 13;     // ASCII linefeed
public boolean init = false;     // ASCII linefeed
Serial myPort;     // Create object from Serial class
ControlP5 cp5;
ScrollableList usbList;
Textarea myTextarea;
Println console;

void setup() {
  size(400, 400);
  cp5 = new ControlP5(this);
  cp5.enableShortcuts();
  myTextarea = cp5.addTextarea("txt")
                  .setPosition(50, 100)
                  .setSize(300, 200)
                  .setFont(createFont("", 10))
                  .setLineHeight(14)
                  .setColor(color(200))
                  .setColorBackground(color(50, 55, 100))
                  .setColorForeground(color(255));
  ;

  console = cp5.addConsole(myTextarea);//
  // create a new button with name 'syncSend'
  cp5.addButton("syncSend")
     .setValue(0)
     .setPosition(200,50)
     .setSize(100,19)
     .setCaptionLabel("Sync with Word Clock");
     ;
  println(Serial.list());
  /* add a ScrollableList, by default it behaves like a DropdownList */
  usbList = cp5.addScrollableList("usbList")
     .setPosition(50, 50)
     .setSize(100, 100)
     .setBarHeight(20)
     .setItemHeight(20)
     .addItems(Serial.list())
     .setCaptionLabel("Select usb port");
     // .setType(ScrollableList.LIST) // currently supported DROPDOWN and LIST
     ;
  usbList.close();   
}

void draw() {
  background(240);
}

public void controlEvent(ControlEvent theEvent) {
  println(theEvent.getController().getName());
  if (theEvent.controller().getName()=="syncSend" && init) sendTimeMessage( TIME_HEADER, getTimeNow());
}

void serialEvent(Serial myPort) {
  char val = char(myPort.read());         // read it and store it in val
    if(val == TIME_REQUEST){
       long t = getTimeNow();
       sendTimeMessage(TIME_HEADER, t);   
    }
    else
    { 
       if(val == LF)
           ; //igonore
       else if(val == CR)           
         println();
       else  
         print(val); // echo everying but time request
    }
    init = true;
}

void usbList(int n) {
  /* request the selected item based on index n */
  println(" Connecting to -> " + Serial.list()[n]);
  myPort = new Serial(this,Serial.list()[n], 115200);
  println(getTimeNow());     
}

void sendTimeMessage(String header, long time) {  
  String timeStr = String.valueOf(time);  
  myPort.write(header);  // send header and time to arduino
  myPort.write(timeStr); 
  myPort.write('\n');  
}

long getTimeNow(){
  // java time is in ms, we want secs    
  Date d = new Date();
  Calendar cal = new GregorianCalendar();
  long current = d.getTime()/1000;
  long timezone = cal.get(cal.ZONE_OFFSET)/1000;
  long daylight = cal.get(cal.DST_OFFSET)/1000;
  return current + timezone + daylight; 
}


