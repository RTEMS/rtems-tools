set startTime [clock seconds]
#puts "Start Time: [clock format $startTime -format %D\ %H:%M:%S]"

# Use `lindex $argv` to get command line arguments

# Initialize variables with default values
set jtag_serial ""
set jtag_ip ""
set cpu 0

# Process command-line arguments
for {set i 1} {$i < $argc} {incr i} {
    set arg [lindex $argv $i]
    switch -- $arg {
        "-j" -
        "--jtag_serial" {
            incr i
            set jtag_serial [lindex $argv $i]
        }
        "--jtag_ip" {
            incr i
            set jtag_ip [lindex $argv $i]
        }
        "-c" -
        "--cpu" {
            incr i
            set cpu [lindex $argv $i]
        }
        "--dtb" {
            incr i
            set dtb [lindex $argv $i]
        }
        "--dtb_address" {
            incr i
            set dtb_address [lindex $argv $i]
        }
        default {
            puts "Warning: Unknown argument $arg"
            exit 2
        }
    }
}

# Connect to fast programmer if required

if { $jtag_ip ne "" } {
    puts "INFO: Connecting to SmartLynq programmer at $jtag_ip:3121"
    connect -url $jtag_ip:3121 
    jtag target 1
    set ffast 10000000
} else {
#    puts "INFO: Connecting to localhost programmer (SN: $jtag_serial)"
    connect 

    # Increase JTAG Target Frequency
    if { $jtag_serial ne "" } {
        jtag targets -set -filter { (name =~ "xcku040" || name =~ "xcku060") && jtag_cable_serial =~ "$jtag_serial" }
    } else {
        jtag targets -set -filter { (name =~ "xcku040" || name =~ "xcku060") }
    }
    set flist [jtag frequency -list]
    set length [llength $flist]
    set ffast [lindex $flist $length-1]
}
# puts "INFO: Programing rate of $ffast bits/sec determined"
jtag frequency $ffast

# Download elf to mb$cpu
if { $jtag_serial ne "" } {
    targets -set -nocase -filter { name =~ "microblaze*#$cpu" && jtag_cable_serial =~ "$jtag_serial" }
} else {
    targets -set -nocase -filter { name =~ "microblaze*#$cpu"   }
}
catch { stop }
after 1000
rst
after 2000

set stopTime [clock seconds]
#puts "End Time: [clock format $stopTime -format %D\ %H:%M:%S]"
#puts "Delta Time: [clock format [expr $stopTime - $startTime] -format %M:%S]"

exit
