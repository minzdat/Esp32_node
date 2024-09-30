#ifndef VERSION_H
#define VERSION_H

// Firmware Version
/*
MAJOR: Increases when there are major changes that may not be backward compatible.
MINOR: Increases when new features are added while maintaining backward compatibility.
PATCH: Increases when minor bug fixes or small changes are made that do not affect backward compatibility.
*/
#define FIRMWARE_VERSION_MAJOR    1
#define FIRMWARE_VERSION_MINOR    1
#define FIRMWARE_VERSION_PATCH    0

// Build number (incremented for each build)
#define FIRMWARE_BUILD_NUMBER     1

// Version as a string
#define FIRMWARE_VERSION_STRING   "1.1.0"

// Changelog
/*
 * Changelog:
 * ----------
 * v1.1.0 -20224-29-08
 * -Release to test housing at farm (30/08/2024)
 * -Feautre 1: Like v1.0.4
 * -Feature 2: Add check bouy feautre (using interrrupt)
 * -Feature 3: Assign unique id for each device
 * -Feature 4: Add version information
 * 
 * v1.0.4 -20224-27-08
 * -Release to test power saving
 * -Feature 1:Power saving(not including automatic light sleep mode)
 * -Feature 2:Using NVS to save data when losing connect to broker
 * -Feature 3:Wifi Provisioning
 * -Feature 4:timestamp synchronization
 * -Feature 5:Using ESP Timer instead of FreeRTOS Software Timer to measure time of Sensor measurement
 * 
 * 
 * v1.0.0 - YYYY-MM-DD
 * - Release to test housing at farm
 * - Feature 1 implemented
 * - Feature 2 implemented
 * 
 * v0.9.0 - YYYY-MM-DD
 * - Release to test Chemins sensor  
 * - Implemented core functionality
 * - Known issue: [Description of known issue]
 * 
 * v0.5.0 (Build 30) - YYYY-MM-DD
 * - Release to first long-run test
 * - Basic framework set up
 * - [List major components or features implemented]
 */

#endif // VERSION_H