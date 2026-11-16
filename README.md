
<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://i.ibb.co/LDNR23jg/MODEM73-white.png">
    <source media="(prefers-color-scheme: light)" srcset="https://i.ibb.co/wZKznzrF/MODEM73-blk.png">
    <img alt="MODEM73" src="https://i.ibb.co/wZKznzrF/MODEM73-blk.png">
  </picture>
</p>


MODEM73 is an open source software modem that works with any HF, VHF, or UHF radio capable of 2400 Hz of bandwidth. All you need is a sound card and audio cable for your radio.  
<p align="center">
<img width="800" height="508" alt="preview_gif" src="https://github.com/user-attachments/assets/53fb54b7-3203-48e4-b8f6-1c9186c645e0" />
</p>
<table align="center">
  <tr>
    <td align="center"><img height="200" alt="modem73 on a UV-K6 with AIOC" src="https://github.com/user-attachments/assets/7180ab80-4386-4ee1-8029-42ca5300ef13" /></td>
    <td align="center"><img height="200" alt="modem73 on a Xiegu X6100 with Armbian" src="https://github.com/user-attachments/assets/5ac2a8bd-75a1-48a4-8264-74a851a06767" /></td>
    <td align="center"><img height="200" alt="modem73 over HF with an IC-7300" src="https://github.com/user-attachments/assets/27fe5f6d-3650-422b-ac52-3ca04b6b1469" /></td>
  </tr>
  <tr>
    <td align="center"><sub>Running on a UV-K6 with an All-In-One Audio Cable (AIOC)</sub></td>
    <td align="center"><sub>Running natively on a Xiegu X6100 with Armbian</sub></td>
    <td align="center"><sub>Running over HF with an ICOM-7300</sub></td>
  </tr>
</table>



SSB, AM, and FM are all supported. It's plug and play compatible with any KISS application and works with rigctl, CM108 sound devices, and serial PTT out of the box.

There are three modem families each suited for covering any possible RF setup from clean line of sight FM links to poor HF band conditions. The receiver decodes all of them at the same time, so one station can hear anything another station sends without switching modes. The modem type can be changed under [ CONFIG ] -> Modem or via the control port  

**OFDM**, based on the open source [COFDMTV modem](https://github.com/aicodix/modem) developed by Ahmet Inan / [aicodix GmbH](https://www.aicodix.de/). Modulations from BPSK to QAM4096 with code rates from 1/4 to 5/6 and payloads from 256 to 6144 bytes per frame. Rates run from about 790 bits per second to over 13 kilobits per second in the same 2400 Hz.

**ROBUST**, a set of slow modes built for fading HF paths such as 40 and 80 meter NVIS.  Five modes from 1150 bps down to 149 bps in either 2400 Hz or a 600 Hz narrow variant.

**MFSK**, a non-coherent mode for weak signal propagation and backup links. 


On VHF/UHF FM, use any OFDM mode.

On good HF SSB paths, use OFDM at 8PSK 1/2  or below depending on your signal budget

On fading/weaker HF paths, use ROBUST modes. For very weak signals or as a backup use MFSK.

### Features

- KISS over TCP so it works with anything that speaks KISS: APRS clients, HamIRC, packet BBS software, Reticulum, custom applications
- JSON control port API for status and configuration for writing your own programs
- Optional VARA-compatible command/data ports so linbpq64 can attach with `DRIVER=VARA` (see `VARA_PORT.md`) — **this fork**
- lightweight UI that runs straight from the terminal and headless mode for embedded use 

## This fork (kg9ni) vs upstream

This repository is **[kg9ni/modem73](https://github.com/kg9ni/modem73)**, a fork of upstream **[RFnexus/modem73](https://github.com/RFnexus/modem73) 2.5.0**.

Use **this fork** when you need the VARA host API and related fixes. Use [upstream](https://github.com/RFnexus/modem73) for stock modem73 without those changes.

What this fork adds on top of 2.5.0:

| Area | Detail |
|------|--------|
| VARA command/data ports | TCP ports for linbpq `DRIVER=VARA` (default **8300** command / **8301** data). Host API only; over-the-air framing is still modem73. See [`VARA_PORT.md`](VARA_PORT.md). |
| `RETRIES` | Host `RETRIES`, CLI `--vara-retries`, and GUI **CONFIG → Network → VARA Retries** share one live connect budget (default 15, matching xara HF). |
| GUI Network settings | **CONFIG → Network** exposes **VARA Port**, **VARA Bind**, and **VARA Retries**. STATUS shows the listen ports and current retries. |
| Busy / retries fixes | Sticky `BUSY ON` after host attach no longer blocks linbpq CONNECT on a quiet channel; configured retries above 15 are honored end-to-end. |

Quick enable:

```
./modem73 --headless --callsign N0CALL --vara-port 8300
```

Or in the TUI: **CONFIG → Network** → set **VARA Port** to `8300` → restart. Full linbpq notes and troubleshooting live in [`VARA_PORT.md`](VARA_PORT.md).

### [5 minute quickstart guide](QUICKSTART.md)

Upstream HTML mirror (general modem73, not fork-specific): https://modem73.app/quickstart.html

## Installation

Clone and build **this fork** (`kg9ni/modem73`). Paths that still say RFnexus below are labeled as upstream-only.

Want to try a browser demo of stock modem73? Upstream WASM demo:  
https://rfnexus.github.io/modem73-wasm-demo/  
Hit "Start microphone" to begin decoding.

### Windows

For **this fork** (VARA ports + `RETRIES` + Network GUI), download the latest kg9ni Windows release:

**[v2.5.2-vara](https://github.com/kg9ni/modem73-win/releases/tag/v2.5.2-vara)** — zip:  
https://github.com/kg9ni/modem73-win/releases/download/v2.5.2-vara/modem73_2.5.2-vara_win64.zip

Release index: https://github.com/kg9ni/modem73-win/releases

How the Windows zip is built and shipped: [`packaging/windows/README.md`](packaging/windows/README.md).

Upstream Windows port (no kg9ni VARA changes): https://github.com/RFnexus/modem73-win | https://github.com/RFnexus/modem73-win/releases/

### Android

Upstream Android port: https://github.com/RFnexus/modem73-android


## Building from source

### Linux

On a system with `apt` or an Arch-based distro with `pacman`? From a clone of **this** repo, run the installer script:
```
./install.sh
```
--- 

1. Install dependencies

```
# Debian/Ubuntu/Pi
sudo apt install git build-essential libncurses-dev g++
```


#### Optional Addons


##### Hamlib PTT
modem73 can talk to your radio through Hamlib directly instead of going through rigctld. Install libhamlib-dev or your distro equivalent before building and the Makefile picks it up. Then set PTT to HAMLIB under [ CONFIG ], pick your rig model, and point it at the serial port or a network rig as host:port. rigctld still works the same as before if you'd rather keep that.

```bash
sudo apt install libhamlib-dev
```

##### CM108 USB PTT Support
CM108-based USB audio interfaces have GPIO pins that can be used for PTT control. To enable CM108 support, install libhidapi-dev before building. The Makefile will auto-detect it and enable the feature.
```
# Debian/Ubuntu/Pi - install before building 
sudo apt install libhidapi-dev
```
----
2. Clone and build



```
# Clone this fork (kg9ni/modem73)

git clone https://github.com/kg9ni/modem73

# Build

cd modem73
make 

# Optional: move to /usr/local/bin 
sudo make install 
```

and run `./modem73`

### macOS

Apple's default bundled ncurses is too old for the UI, so install Homebrew's and build with the same `make`:

```
brew install ncurses
git clone https://github.com/kg9ni/modem73
cd modem73
make
```

The Makefile picks up Homebrew's ncurses through `brew --prefix ncurses`.

if your Homebrew lives elsewhere pass the prefix explicitly with `make NCURSES_PREFIX=/path/to/ncurses`. Hamlib and hidapi from Homebrew are detected the same way as on Linux when `pkg-config` can see them.

## Running & Operations

By default, MODEM73 will listen on port 8001

All of the modes provided by the OFDM modem require a bandwidth of 2400 Hz and work over both FM and SSB. 

There are currently six PTT options:
- NONE (speaker/mic over the air)
- Hamlib (For Hamlib supported rigs)
- Rigctl
- VOX
- Serial
- CM108


```
# Start in UI mode
./modem73

# Start in headless mode
./modem73 --headless

# See all options with:
./modem73 --help
```

### Selecting a mode

```
# OFDM (default), pick modulation, code rate and frame size
./modem73 --head-m QAM16 -r 3/4 --normal

# ROBUST, suffix S for short frames
./modem73 --robust-mode RDM-600S

# MFSK
./modem73 --mfsk-mode MFSK-16
```

### CSMA

```
# Pick the busy detector, then a band and preset for the timings
./modem73 --csma-mode sync --csma-band vhf --csma-preset moderate

# Individual knobs override the preset when they come after it
./modem73 --headless --csma-band hf --csma-preset relaxed --csma-burst 3
```

### TX audio level

```
# Soundcard output drive, 5-100 percent
./modem73 --tx-level 60
```


### PTT options 

```
# Connect to rigctld for PTT control
./modem73 --rigctl localhost:4532
```

while running `rigctld`


```
./modem73 --ptt vox --vox-freq 1200 --vox-lead 500 --vox-tail 150
# 500ms vox lead and 150ms vox tail
```


```
./modem73 --ptt com --com-port /dev/ttyUSB0 --com-line rts
```

```
# CM108 USB audio interface PTT (GPIO3 is the default)
./modem73 --ptt cm108 --cm108-gpio 3
```

### Control port

A control port for modem73 will automatically start on port `8073` by default. View `CONTROL_PORT.md` for the full JSON spec

### VARA port (linbpq64)

To let linbpq64 attach with `DRIVER=VARA` (command 8300 / data 8301):

```
./modem73 --headless --callsign N0CALL --vara-port 8300
```

`RETRIES` defaults to 15 (xara HF); override with `--vara-retries N` or the host `RETRIES` command. See `VARA_PORT.md`.

## Usage

### All In One Audio Cable (AIOC)
modem73 supports the [AIOC](https://github.com/skuep/AIOC) out of the box. To use the All In One Audio cable, set PTT to COM, specify your COM port, and set PTT line to `BOTH` and Invert to `INVERT RTS`. Make sure you have the correct permissions and `/dev/xxxx` specified. The AIOC on most setups will be /dev/ttyACMx (where x is 0, 1, 2). Note that it may change after a device restart, plugging it back in, etc. 

### rigctl
modem73 supports Hamlib and rigctl for any rigctl supported radio for PTT. Set rigctl to your options and run `rigctld -m (your model) -s (serial baud rate) -r /dev/XXXX)`  The `d` at the end of `rigctl` tells rigctl to run in network mode, which is what modem73 will connect to.

### Reticulum
Want to use modem73 with Reticulum? Check out the upstream modem73interface:  
https://github.com/RFnexus/modem73interface

Drop it into `~/.reticulum/interfaces/` and in your Reticulum config, add something like:
```
[[MODEM73]]
type = Modem73Interface
enabled = yes
target_host = 127.0.0.1
target_port = 8001
control_host = 127.0.0.1
control_port = 8073
```


## Platform Specific Notes

### Pi Zero 2

If you are building from source on a memory constrained device like the Pi Zero 2, you will need to increase swap space. On the Pi Zero 2, and similar this can be done with:

 ```bash
  sudo fallocate -l 2G /swapfile
  sudo chmod 600 /swapfile
  sudo mkswap /swapfile
  sudo swapon /swapfile
  free -h                    # confirm the 2G appeared
 ```

The Pi Zero 2 with its SoC can only support 1 decoder at a time. If you are tight on CPU, turn off the decoders you aren't using under CONFIG. 

## Updating

modem73 includes an update utility `update.sh` that pulls from **this fork** (`kg9ni/modem73`).

To update to the latest version of this fork:
```
./update.sh
```
