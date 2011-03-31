#!/usr/bin/perl

use strict;
use warnings;
use YAML::XS;

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
print "$test_cmd -g $gri -S \"$src_urn\" -D \"$dst_urn\" -B $bandwidth" . "M -s 0 -e 0 -u $src_vlan -v $dst_vlan\n";
my $result = `$test_cmd -g $gri -S $src_urn -D $dst_urn -B $bandwidth . M -s 0 -e 0 -u $src_vlan -v $dst_vlan`;
print $result;
print "\n";

1;
