# IR_RemoteController_WLED
Use tv remote signal to send commands to WLED using ESP-NOW.



1. find your ESP's MAC adresses (WLED Esp and remote ESP).
2. Add the WLED MAC to the .ino
3. Add the remote MAC to WLED>WiFi-Preferences (Enable ESP-NOW)

   Note:
   - Some TV-remotes send 2-3consecutive signals to turn ON/OFF the TV. I added a 500 delay for this, it might need to be updated for other remotes.
   - You can use this code to find out the HEX values that your TV-remote sends out and add your own logic to my code.

Credits:
-https://github.com/AlLaguna/ESP-NOW-WLED-Controller
