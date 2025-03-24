## Notes on this fork
- CMake Project with Linux Support
- Physical Memory access through pseudo Process "Physical Memory"
- Write Access to separte Process tagged with [WRITE ACCESS] in Process list, to not make a write by mistake
- Exposed EPROCESS and CR3 in Sections of Process

# ReClass.NET-PciLeechPlugin
A plugin that integrates vmm.dll from the https://github.com/ufrisk/MemProcFS project to allow ReClass.NET to function over a PCIe FPGA device.

## Usage

* Copy `PciLeechPlugin.dll` into the ReClass.NET\x64\Plugins directory
* Copy `leechcore.dll`, `vmm.dll`, `FTD3XX.dll`, and `mmap.txt` from MemProcFS into the ReClass.NET\x64 directory
* Open Reclass.NET, go to File -> Plugins
* Switch to the Native Helper tab and change the Functions Provider from Default to PciLeechPlugin
