#!/usr/bin/perl

use strict;
use warnings;
use YAML::XS;
use Time::ParseDate;

my $test_cmd =  '../src/main/mxtce_test';
my $yaml_file = shift;
my $config = do{local(@ARGV,$/)=$yaml_file;<>};
my $request = Load $config;

print '-' x 80 . "\n";
my $gri  = $request->{'create'}{'gri'};
my $src_urn = $request->{'create'}{'src'};
my $dst_urn = $request->{'create'}{'dst'};
my $bandwidth = $request->{'create'}{'bandwidth'};
my $src_vlan   = $request->{'create'}{'srcvlan'};
my $dst_vlan   = $request->{'create'}{'dstvlan'};
my $start_time   = $request->{'create'}{'start-time'};
my $end_time   = $request->{'create'}{'end-time'};
my $path_type   = $request->{'create'}{'path-type'};
$start_time = parsedate($start_time);
if ($end_time =~ /\+(\d+):(\d+):(\d+)/) {
	$end_time = $start_time + $1*3600*24 + $2*3600 + $3*60;
} else {
	$end_time = parsedate($end_time);
}
my $cmd = "$test_cmd -g $gri -S \"$src_urn\" -D \"$dst_urn\" -B $bandwidth" . "M -s $start_time -e $end_time -u $src_vlan -v $dst_vlan\n";
print "$cmd \n";
my $result = `$cmd`;
print $result;
print "\n";

1;
