# SCPI Command Reference for PICO APG

| Command | Parameters | Description | Details | Default | Status |
|---|---|---|---|---|---|
| `:SOURce:PWM:MODE`<br>`:SOURce:PWM:MODE?` | `OFF\|ONEPH\|TWOPH\|THREEPH` | Set/Query PWM operating mode | OFF: disables PWM<br>ONEPH: single phase<br>TWOPH: two-phase<br>THREEPH: three-phase | OFF | planned |
| `:SOURce:PWM:CONTrol`<br>`:SOURce:PWM:CONTrol?` | `DUTY\|MOD_ANGLE\|MOD_SPEED` | Set/Query control mode | DUTY: set DUTY cycle directly<br>MOD_ANGLE: set MODulation index & phase ANGLE<br>MOD_SPEED: set MODulation index & phase rotation SPEED<br>In ONEPH mode, only DUTY control is available. | DUTY | planned |
| `:SOURce:PWM:FREQuency`<br>`:SOURce:PWM:FREQuency?` | `<frequency>` | Set/Query PWM carrier frequency | Frequency in Hz<br>MIN=1.0, MAX=100000.0 | 10000.0 | planned |
| `:SOURce:PWM:ALIGNment`<br>`:SOURce:PWM:ALIGNment?` | `CENTER\|EDGE` | Set/Query PWM alignment | CENTER: center-aligned<br>EDGE: leading-edge aligned | CENTER | planned |
| `:SOURce:PWM:DEADtime`<br>`:SOURce:PWM:DEADtime?` | `<deadtime>` | Set/Query PWM deadtime | Deadtime in nanoseconds between switching<br>MIN=0, MAX=10000 | 1000 | planned |
| `:SOURce:PWM:PHase<n>:DUTY`<br>`:SOURce:PWM:PHase<n>:DUTY?`<br>n=1-3 | `<duty>` | Set/Query duty-cycle | Use fraction (0.0 to 1.0)<br>0.0 = LS always on, 1.0 = HS always on<br>MIN=0.0, MAX=1.0 | 0.5 | planned |
| `:SOURce:PWM:MINDuty`<br>`:SOURce:PWM:MINDuty?` | `<min>` | Set/Query minimum duty cycle | Maximum is symmetrically limited to (1.0 - MIN)<br>Will always be enforced, even in MOD control modes<br>MIN=0.0, MAX=0.2 | 0.05 | planned |
| `:SOURce:PWM:MOD`<br>`:SOURce:PWM:MOD?` | `<mod>` | Set/Query modulation index | Modulation index for SPWM (0.0 to 1.0)<br>The generated duty cycle will be: 0.5 + 0.5 * MOD * sin(angle), but capped to respect MIN/MAX duty cycle.<br>MIN=0.0, MAX=1.0 | 0 | planned |
| `:SOURce:PWM:ANGLE`<br>`:SOURce:PWM:ANGLE?` | `<angle>` | Set/Query SPWM angle | Phase angle in degrees (wraps at 360°)<br>0° = Phase 1 high | 0 | planned |
| `:SOURce:PWM:SPEED`<br>`:SOURce:PWM:SPEED?` | `<speed>` | Set/Query SPWM rotation speed | Rotation speed of SPWM phase in RPM<br>MIN=0, MAX=100000 | 100 | planned |
| `:SOURce:PWM:PHase<n>:LS`<br>`:SOURce:PWM:PHase<n>:LS?`<br>n=1-3 | `<gpio>` | Set/Query low-side GPIO assignment | GPIO pin number<br>MIN=0, MAX=29 | 0 | planned |
| `:SOURce:PWM:PHase<n>:HS`<br>`:SOURce:PWM:PHase<n>:HS?`<br>n=1-3 | `<gpio>` | Set/Query high-side GPIO assignment | GPIO pin number<br>MIN=0, MAX=29 | 0 | planned |
| `:SOURce:PWM:CHANnel:INVert`<br>`:SOURce:PWM:CHANnel:INVert?` | `<bool>` | Set/Query PWM channel output inversion | ON: inverts output polarity<br>OFF: normal | False | planned |
