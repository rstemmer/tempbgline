
# tempbgline

This tool prints an unicode bargraph that displays the temperature of the CPUs cores.
On a hexacore, this may look like this: `▅▆▅▆▅▅`

Only CPUs are supported, where the temperature sensors are listed at _/sys/devices/platform/coretemp.*/hwmon/hwmon*_.
If this directory structure is not available on your system, you might be able to use the -m switch as workaround.
You possible have to modify the source code if the are some files missing inside the monitors directory.


## Installation

You should check the `install.sh` script before executing.
The default installation path is _/usr/local/bin_

```bash
# Download
git clone https://github.com/rstemmer/tempbgline.git
cd tempbgline

# Build
./build.sh

# Install
sudo ./install.sh
```

## Usage

tempbgline [-v|-a|-m PATH]

 * -v: Be verbose and list all captured informations
 * -a: Show only one bar for the average temperature of all cores
 * -m: Path to the _hwmon*_ directory. This prevents searching the sensor values at each call.

### Examples

```bash
watch -n 1 tempbgline
```

See the `tmux.conf` file for an example usecase. There the CPU core temperature gets printed into the status line.

## How it works

The tool searches the temperature sensor values by looking at _/sys/devices/platform_ for a directory called _coretemp.x_ where x is an integer.
Inside the first directory (lowest available _x_) the tool looks for a subdirectory _hwmon/hwmony_ with y as another integer.
The first directory found (lowest y) will be choosen as monitor from which the temperature data gets captured.
This process can be skipped by using the _-m_ option and setting the directory explicitly.
This may also fix issues in case a wrong hardware monitor gets choosen.

In a second step, all tempX files gets read. Those files contain labels, temperature thresholds and the actual temperature of each core and in some cases the package.

Last, the captured values get processed and printed.
All temperatures that are not related to a core (their lable is "Core X" with X as core ID) are ignored.
The rest gets mapped to one of 8 block characters of the unicode, and the space character.
The math behind this is:

```c
float reltemp = core->temp / temp->hightemp;
int   tilenr  = (int)(8 * reltemp + 0.5);
```

So the relative temperature is calculated using the high-temperature threshold. This value can be more than 1.0 because there is a critical-temperature threshold.
Is the relative temperature greater than 1.0 a "!" gets printed.

The average temperature is calulated in a similar way:

```c
while(core)
{
    sum += core->temp / core->hightemp;
    core = core->next;
}
int tilenr = (int)(8 * (sum/numofcores) + 0.5);
```

Critical temperatures may vanish if only one core is slightliy to high.
If the average temperature is critical the graph will also display a "!".

