#!/usr/bin/perl
# SpaceNavigator Test
#
# setup: http://code.google.com/p/liquid-galaxy/wiki/LinuxSpaceNavigator
# short: add to udev rules: KERNEL=="event[0-9]*", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c62[68]", MODE="0664", GROUP="plugdev", SYMLINK+="input/spacenavigator"
# needs evtest tool
my %h;
open(SN,"evtest /dev/input/spacenavigator |") or die "evtest failed";
print "press both buttons for exit\n";
while(<SN>) {
    chomp;
    next if not (/EV_REL/ or /EV_KEY/);
    @_=split(/, /,$_);
    $_[2]=~ s/.*?_//;
    $_[2]=~ s/\).*//;
    $_[3]=~ s/.*?lue //;
    $h{$_[2]}=$_[3];
    printf("X %4d Y %4d Z %4d RX %4d RY %4d RZ %4d | BTN_0 %d BTN_1 %d\n", 
	$h{"X"}, $h{"Y"}, $h{"Z"}, 
	$h{"RX"}, $h{"RY"}, $h{"RZ"}, 
	$h{"0"}, $h{"1"});
    last if $h{"0"} == 1 and $h{"1"} == 1;
}
close(SN);
