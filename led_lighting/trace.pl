#/usr/bin/perl -w

use strict;

my @lst;
my %targ;
my %ins;

my $cnt=0;
my $max_cycles = 0;

sub run {
    my $pc = shift;
    my $cycles = shift;
    my $indent = shift;
    my $taken;
    my $not_taken;
    my $op;
    my $args;
    
    while(1) {
	$op = $lst[$pc];
	$op =~ s/^\s+//;
	$op =~ s/\s+.*//;
	$args = $lst[$pc];
	$args =~ s/^\s+//;
	$args =~ s/^\S+//;
	$args =~ s/^\s+//;
#	($op, $args) = split(/^\s+\S+\s+/,$lst[$pc]);
	@_ = split(/ /,$ins{$op});
	($not_taken, $taken) = split(/\//,$_[0]);
	print " "x$indent;
	print "$pc: $op $args ($cycles + $_[0])\n";
	if(not defined $ins{$op}) {
	    die "instruction undefined: $op";
	}
	elsif($_[1] eq "end") {
	    $cycles += $_[0];
	    print " "x$indent;
	    print "=> $cycles cycles\n";
	    $max_cycles = $cycles if $cycles > $max_cycles;
	    return;
	}
	elsif($_[1] eq "jmp") {
	    $cycles += $_[0];
	    die "target $args undefined" if not defined $targ{$args};
	    print " "x$indent;
	    print "$args:\n";
	    $pc = $targ{$args};
	}
	elsif($_[1] eq "skip") {
	    die if not defined $taken;
	    die if not defined $not_taken;
	    run($pc+2, $cycles + $taken, $indent+2);
	    $pc++;
	    $cycles += $not_taken;
	}
	elsif($_[1] eq "branch") {
	    die if not defined $taken;
	    die if not defined $not_taken;
	    die "target $args undefined" if not defined $targ{$args};
	    print " "x($indent+2);
	    print "$args:\n";
	    run($targ{$args}, $cycles + $taken, $indent + 2);
	    $pc++;
	    $cycles += $not_taken;
	}
	else {
	    $cycles += $_[0];
	    $pc++;
	}
    }
}

while(<STDIN>) {
    chomp;
    s/;.*//;
    s/\s+$//;
    next if not /\S+/;
    if(/^\S+:/) {
	s/:.*//;
	$targ{$_}=$cnt;
	#print "$_:\n";
    }
    elsif(not /^\./) {
	s/^\s+//;
	$lst[$cnt]=$_;
	#print "$cnt $_\n";
	$cnt++;
    }
}

open(INS,"ins.txt");
while(<INS>) {
    chomp;
    @_=split(/\s+/, $_);
    $ins{$_[0]} = "$_[1] $_[2]";
    #print "$_[0] -> $_[1] $_[2]\n";
}
close(INS);

run(1,4,0);
print "max: $max_cycles cycles\n";

