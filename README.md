# OpenCV
#  Detecting and counting smiles and their intensitie\
This project counts the number of smiles and their intensities.\
A fingerprint model is connected to a PIC18f4550 through UART (Tx, Rx) and the Pic is connected back to the Raspberry Pi through GPIO. The finger print is used as a way of authentication.\
The system is controlled by a 4x4 keypad connected to a Raspberry Pi 3 Model B.\
Two LEDs are also connected to the R pi. One blinks while the video is playing and the other one blinks when a screenshot is taken.\
Information is then stored in a database.

# Equipment
- Raspberry Pi model B
- 2x LEDs
- 4x4 keypad
- PIC18f4550
- FPM10A Optical Fingerprint Sensor
