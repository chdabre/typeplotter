import controlP5.*;
import java.awt.*;
import processing.serial.*;

ControlP5 cp5;
Serial plotter;

PImage source, destination;

String serPort = "";

int printSize = 0;
int printSpeed = 0;
int dither = 0;
int passes = 1;

boolean invertState = false;

int lineY = -1;

int x = 0;

boolean printing = false;

void setup() {

  size(600, 600);
  background(0);
  image(loadImage("splash1.png"), 0, 0);

  // CONTROL P5 INIT ----------------------------------------------------------------------------------------------  
  cp5 = new ControlP5(this);


  cp5.addSlider("printSize")
    .setPosition(2, 0)
      .setRange(1, 16)
        .setValue(1) 
          .setId(0)
            ;
  cp5.addSlider("dither")
    .setPosition(2, 11)
      .setRange(0, 8)
        .setValue(4) 
          .setId(4)
            ;

  cp5.addButton("invert")
    .setValue(10)
      .setPosition(2, 22)
        .setSize(99, 10)
          .setId(5);

  cp5.addButton("PRINT!")
    .setValue(10)
      .setPosition(152, 12)
        .setSize(100, 10)
          .setId(1);

  DropdownList dl = cp5.addDropdownList("Serial Port").setPosition(152, 10).setSize(200, 200).setId(2);
  for (int i=0; i<Serial.list ().length; i++) {
    dl.addItem(Serial.list()[i], i);
  }

  DropdownList dlSpeed = cp5.addDropdownList("Print Speed").setPosition(256, 22).setSize(96, 100).setId(3);
  dlSpeed.addItem("Ultra", 80);
  dlSpeed.addItem("Draft", 100);
  dlSpeed.addItem("Normal", 200);
  dlSpeed.addItem("Beautiful", 350);

  // END CONTROL P5 INIT ----------------------------------------------------------------------------------------------

  selectInput("Select a file to process:", "fileSelected");
}

void fileSelected(File selection) {

  if (selection == null) {
    println("Window was closed or the user hit cancel.");
    exit();
  } else {
    source = loadImage(selection.getAbsolutePath());
    resizeImages();

    dither = 8;
    reDither();
    dither = 4;
    reDither();
  }
}

void draw() {
  if (printing)cp5.hide();
  if (destination!=null)image(destination, 0, 0); //set preview image

  fill(0, 179, 237);
  noStroke();
  if (!printing)rect(2, 0, 148, 22); // cp5 background rectangle

  stroke(255, 0, 0);
  line(0, lineY, width, lineY); // draw status line
}

public void controlEvent(ControlEvent theEvent) {

  if (theEvent.isGroup()) {

    if (theEvent.getGroup().getId() == 3) {
      printSpeed = int(theEvent.getGroup().getValue()); // print speed dropdown
    } else if (theEvent.getGroup().getId() == 2) {
      serPort = Serial.list()[int(theEvent.getGroup().getValue())]; // serial port dropdown
    }
  } else if (theEvent.controller().id() == 1) {
    printImage(); // start print
  } else if (theEvent.controller().id() == 4 || theEvent.controller().id() == 5) {
    if (theEvent.controller().id() == 5) invertState=!invertState;
    reDither();
  }
}

void printImage() {

  // check if all parameters are valid
  if (serPort == "") {
    javax.swing.JOptionPane.showMessageDialog(null, "please select a valid serial port!");
    return;
  }

  if (printSpeed == 0) {
    javax.swing.JOptionPane.showMessageDialog(null, "please select a printing speed!");
    return;
  }

  println("Width in steps: "+destination.width*printSize);
  if (destination.width*printSize > 2500) {
    javax.swing.JOptionPane.showMessageDialog(null, "paper too small, setting print size to the maximum of " + round(2500/destination.width));
    printSize = round(2500/destination.width);
  }
  printing = true;
  javax.swing.JOptionPane.showMessageDialog(null, "Make sure the Arduino at "+serPort+" is ready!"); // show

  plotter = new Serial(this, serPort, 9600);
  plotter.bufferUntil('\n');

  plotter.write(destination.width+","+destination.height+","+int(printSize)+","+int(printSpeed)); // send initialising data to plotter

  println("width: "+destination.width+" height: "+destination.height+" Print Size: "+int(printSize)+" Print Speed: "+int(printSpeed));

  printing = true;
  lineY = 0;
}

void serialEvent(Serial plotter) {
  if (printing) {

    int pixel = (brightness(destination.pixels[x]) == 0) ? 1 : 0;

    plotter.write(pixel);
    x++;

    if (x % destination.width == 0) {
      lineY++;
    }

    if (x == destination.width * destination.height) {
      printing = false;
      javax.swing.JOptionPane.showMessageDialog(null, "Done!");
      exit();
    }
  }
}

PImage getDitheredImage(PImage img, int n, boolean invert) {
  PImage out = createImage(img.width, img.height, RGB);

  img.loadPixels();
  out.loadPixels();

  float f = 255 / (pow(2, 2*n) + 1);

  for (int x = 0; x < img.width; x++) {
    for (int y = 0; y < img.height; y++) {

      color c = img.get(x, y);
      float t = (n > 0) ? dizza(x, y, n) * f : 128;

      if (invert) {
        c = color( (t >= brightness(c)) ? 255 : 0 );
      } else {
        c = color( (t >= brightness(c)) ? 0 : 255 );
      }

      out.pixels[ x + y*img.width ] = c;
    }
  }
  out.updatePixels();

  return out;
}

int dizza(int i, int j, int n) {
  if (n==1) {
    return (i%2!=j%2 ? 2 : 0) + j%2;
  } else {
    return 4 * dizza( i % 2, j % 2, n-1) + dizza( int( i / 2 ), int( j / 2 ), n - 1);
  }
}

void resizeImages() {
  frame.setResizable(true);
  if (source.width < width || source.height < width) {
    frame.resize(source.width, source.height+20);
  } else if (source.width>width) {
    source.resize(width, (width/source.width)*source.height);
    frame.resize(source.width, source.height);
  } else if (source.height>height) {
    source.resize(height, (height/source.height)*source.width);
    source.resize(source.width, source.height);
  }

  frame.setResizable(false);
}

void reDither() {
  destination = getDitheredImage(source, dither, invertState); //Redithers image
  destination.save("dest.png");
}

