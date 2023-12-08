# attendance_tracker
Final Project for ECE495: Attendance Tracker

Contains code for the ultrasonic sensor, RFID methods, as well as the Flask application used.

**RFID:**
- rfid.ino
  - Upload student RFID tag to Flask database
- rfid_read.ino
  - Read data stored in memory of an RFID Tag of specific block and sector numbers
- rfid_write.ino
  - Write data from user input to memory of an RFID Tag in a specific block and sector 

**Ultrasonic Sensor:**
- ultrasonic.ino
  - Control ultrasonic sensors and send count information to APIs

**Flask Application:**
- live_tracker.py
  - Flask Application that contains endpoints for updating attendance tables and displaying students
    - Index - displays all records in table and contains flagging function
    - /present - displays all present students 
    - /delete - creates CSV file and clears all tables 
    - /num - returns current number of present students
    - /update - updates student attendance table
    - /update_count - updates current ultrasonic sensor count
- students_tracker.html
  - HTML template for index

For more information: https://docs.google.com/document/d/19cwqqNru1oxK0gD025xdpyEEmbXMzj_3uz8Fu3Qng0w/edit?usp=sharing

