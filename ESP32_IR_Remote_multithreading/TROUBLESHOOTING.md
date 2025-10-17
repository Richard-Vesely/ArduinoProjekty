# Troubleshooting Guide

## Common Compilation Errors and Fixes

### 1. IRremote Library Issues

**Error:** `'IrReceiver' was not declared in this scope`
**Fix:** Update platformio.ini to use IRremote version 3.5.0 or newer

**Error:** `'ENABLE_LED_FEEDBACK' was not declared`
**Fix:** Add `#define ENABLE_LED_FEEDBACK` before including IRremote.hpp or remove this parameter

### 2. FreeRTOS Issues

**Error:** `'xPortGetCoreID' was not declared in this scope`
**Fix:** Already included in main.cpp: `#include <freertos/FreeRTOS.h>`

**Error:** `'pdMS_TO_TICKS' was not declared`
**Fix:** Replace `vTaskDelay(pdMS_TO_TICKS(X))` with `vTaskDelay(X / portTICK_PERIOD_MS)`

### 3. Adafruit NeoPixel Issues

**Error:** Constructor initialization problems
**Fix:** The LED controller uses proper initialization in the constructor member list

### 4. Multiple Definition Errors

**Error:** `multiple definition of 'sharedData'`
**Fix:** Ensure only `src/shared_data.cpp` defines the global instance, others should use `extern`

## Quick Fixes

### If you get IRremote errors:
1. Delete the `.pio` folder in your project
2. Run `pio lib install` to reinstall libraries
3. Try compilation again

### If you get linking errors:
Check that all `.cpp` files are in the `src/` folder and all `.h` files are in `include/`

### If upload fails:
1. Hold the BOOT button on ESP32 during upload
2. Check the COM port in Device Manager (Windows)
3. Add to platformio.ini: `upload_port = COMx` (replace x with your port number)

## Specific Error Messages

Please share the specific error messages you're seeing, and I'll provide targeted fixes. Look for:
- File names and line numbers
- Error type (compilation error, linking error, upload error)
- Any missing symbols or undefined references

## Example Error Format

```
src/main.cpp:20:5: error: 'IrReceiver' was not declared in this scope
     IrReceiver.begin(IR_RECEIVE_PIN);
     ^~~~~~~~~~
```

With this information, I can provide exact fixes for your specific issues.
