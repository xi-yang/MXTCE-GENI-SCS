#!/usr/bin/perl

use strict;
use warnings;
use Lib::TLV;
use Lib::APIClient;
use Time::ParseDate;


use constant API_MSG_RESV_PUSH => 0x0011;

use constant MSG_TLV_RESV_INFO => 0x0011;
use constant MSG_TLV_PATH_ELEM => 0x0012;

sub parse_resv_list($)
{
	my $text = shift;
	my @resvs;
	foreach my $line (split(/\n/, $text)) {
		my $resv = $resvs[-1];
		if ($line =~ /GRI:\s([^\s]+)/) {
			my %resvMem;
			$resv = \%resvMem;
			$resv->{gri} = $1;
			push(@resvs, $resv);
		} elsif ($line =~ /Status:\s([^\s]+)/) {
			$resv->{status} = $1;
		} elsif ($line =~ /startTime:\s([^\s])+\s([^\s]+\s[^\s]+\s[^\s]+\s[^\s]+\s[^\s]+)/) {
		my $start = parsedate($resv->{start});
			$resv->{start} = parsedate($2);
		} elsif ($line =~ /endTime:\s([^\s])+\s([^\s]+\s[^\s]+\s[^\s]+\s[^\s]+\s[^\s]+)/) {
			$resv->{end} = parsedate($2);
		} elsif ($line =~ /bandwidth:\s([^\s]+)/) {
			$resv->{bw} = $1*1000000;
		}
	}
	return @resvs;
}

sub get_resv_path($)
{
	my $resv = shift;
	my $cmd = "cd $ENV{OSCARS_DIST}/api; . bin/setclasspath.sh; java net.es.oscars.api.test.IDCTest -v 0.6 -a x509 -c query --gri " . $resv->{gri};
	my $query_result = `$cmd`;
	my $start = 0;
        foreach my $line (split(/\n/, $query_result)) {
		if ($line =~ /Hops in reserved path are/) {
			$start = 1;
		} 
		next unless ($start);
		if ($line =~ /(urn:ogf:[^\s]+)\s[^\s]+\s([^\s]+)/) {
                        if (!defined($resv->{path})) {
                                my @path;
                                $resv->{path} = \@path;
                        }
                        my %elem;
                        $elem{urn} = $1;
                        $elem{swtype} = 51;
                        $elem{enc} = 2;
                        $elem{vlan} = $2;
			if ($2 eq "null") {
                        	$elem{swtype} = 1;
                        	$elem{enc} = 2;
				$elem{vlan} = 0;
			}
                        push(@{$resv->{path}}, \%elem);
                }
        }
}

die 'Abort: env variable $OSCARS_DIST undefined...' unless (defined($ENV{OSCARS_DIST}));

my $oscars_cmd =  "cd $ENV{OSCARS_DIST}/api; . bin/setclasspath.sh; java net.es.oscars.api.test.IDCTest -v 0.6 -a x509 -c list";
my $interval = 120; 

my $api_conn = new APIClient('sportster', '2091', 10101);
$api_conn->connect_server();

while(1) {
	my $resv_list_text = `$oscars_cmd`;
	#parse text into @resvs (each a \%resv={gri, status, start, end, bw, \@path}
	my @resvs = parse_resv_list($resv_list_text);
	my $total = 0;
	foreach my $resv (@resvs) {
		next unless ($resv->{status} eq "RESERVED" || $resv->{status} eq "ACTIVE");
		$total++;
	}
	my $ct = 1;
	foreach my $resv (@resvs) {
		# skip resv of unqualified status
		next unless ($resv->{status} eq "RESERVED" || $resv->{status} eq "ACTIVE");
		get_resv_path($resv);
		my $resv_info_tlv = new TLV(MSG_TLV_RESV_INFO, $resv->{gri}, $resv->{start}, $resv->{end}, $resv->{bw}, $resv->{status}); 
		my @path_elem_tlvs;
		foreach my $elem (@{$resv->{path}}) {
			my $elem_tlv = new TLV(MSG_TLV_PATH_ELEM, $elem->{urn}, $elem->{swtype}, $elem->{enc}, $elem->{vlan});
			push(@path_elem_tlvs, $elem_tlv);
		}
		my $opts = 0;
		# indicate start of update burst
		if ($ct == 1) {
			$opts = ($opts | 0x0001);
		} 
		# indicate end of update burst
		if ($ct == $total) {
			$opts = ($opts | 0x0002);
		}
                $api_conn->queue_bin_msg(API_MSG_RESV_PUSH, $opts, $resv_info_tlv, @path_elem_tlvs);
		$ct++;
                $api_conn->send_bin_msg();
		print %{$resv};
		foreach my $elem (@{$resv->{path}}) {
			print "\t";
			print %$elem;
			print "\n";
		}
		print  "\n";
	}
	print "sleep $interval seconds\n";
	sleep($interval);
}

1;
