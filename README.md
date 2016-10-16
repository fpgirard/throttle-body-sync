# throttle-body-sync

Uses Freescale MPXV7025DP: https://goo.gl/Sb4YQy
Implements Freescale Best Practices: https://goo.gl/YfErPb
        
VOUT = VS x (0.018 x P + 0.92) ± (Pressure Error x Temp Multi x 0.018 x VS)

IOW:  -25kPa = .1v or 0
        0kPa = 2.3v or 512    
       25kPA = 4.6v or 1023
