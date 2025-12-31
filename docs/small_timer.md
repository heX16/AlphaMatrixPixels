# Small Timer

The library provides software timers for creating non-blocking delays in Arduino and other microcontrollers.

## Main API

**`start(time)`** - Starts the timer for the specified time (in milliseconds). If the timer is already running, restarts it.

**`startOnce(time)`** - Starts the timer only if it is stopped.

**`stop()`** - Stops the timer.

**`run()`** - Main function that must be called in every program cycle. Returns `true` once when the timer has fired. After firing, the timer automatically stops. **IMPORTANT**: The `run()` function must be called only **ONCE** in each program cycle for each timer. _Do not_ call it twice.

## Usage Example

```cpp
csTimer tBlink;

void setup() {
  tBlink.start(1000);  // Start timer for 1 second
}

void loop() {
  if (tBlink.run()) {  // Check if timer has fired
    // Code executes every second
    tBlink.start(1000);  // Restart timer
  }
}
```

