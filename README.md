# Machine-to-Machine
Raspberry Pi Project
The Control system was implemented on a single Raspberry Pi with both, control logic and encoder logic running on it. 
Then, each was connected to different Pis, with the Pis communicating between each other to transfer valuable data for calculation. 
The encoder was able to determine the frequency and direction with the implementation of the real-time encoder program and the controller was able to attain stability for the DC motor using a PID control logic written into the controller program. 
The approximate coefficients for the PID logic were provided after simulations and then were tweaked using the trial-and-error method after running the motor using the programs.
