# OpenXHC-WHB04


this is a rebuild of the Repository https://github.com/moonglow/openxhc 

This Repository consists all the information and code you need to build this Projekt. It is build in Keil UVision and supports varios Displays.

I Rebuild this Projekt in Cube Ide and changed some things.
- This Project is only for HB04, HB03 got removed.
- Encoder on 5V Tollerant Pin on Tim2.
- Supports only ST7735 for now
- Different UI
- Works with STM32 F103c8t6 aka Bluepill

I tested the functionality with Mach 4 Demo ( no Hardware owned) and Beamicon2 so far. But it should work with any software that recognices the Original HB04.
The Buttons can be remapped to your likings in code. There are more values that need customization between software. Like the Percentage bar Values of Feed and Speed overwrite. 
They differ from Software to Software. I startet to make a user_define.h File, where the important values can be changed easily.

Please be aware that this is still a work in Progress, and there can be bugs in the code. So use it with caution. If you recognize any bugs, fell free to report them.


I build a custom PCB, and 3d Printed enclosure. 
The top cover foil is made with simple tools. It is not perfekt, but as I added some domes with a heated river, it gives the Buttons a nice Click.

The Pcb comes assembled from Jlcpcb for around 10€ a piece, but min. order quantity is 5.
The pushbuttons, Rotary and Display are handsoldered.

This is still a work in progress, as the encoder output, and the UI need a little bit of cleanup.
But all the functions work properly, and i use it on my machine.

This Projekt will be updated whiule I work on it to get it finished.
Here are some Impressions of the projekt in its current state:

## Finished Hardware V1.0
![v1 0_build_finished](https://github.com/user-attachments/assets/1b9d413c-b84d-428c-9bd9-02228abece8d)

## Pcb V1.0
![pcb_V1 0](https://github.com/user-attachments/assets/5d2cdaf9-2615-4f56-9971-138232a727e2)

## a Look inside
![cover_open](https://github.com/user-attachments/assets/40c11b59-10a0-4417-a93a-418ff70080a0)

## Display Ui
![ui](https://github.com/user-attachments/assets/c57479b0-7ce7-49dd-b0fe-c94b38cec598)

