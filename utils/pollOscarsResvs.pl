#!/usr/bin/perl

use strict;
use warnings;
use Lib::TLV;
use Lib::APIClient;


BEGIN {
        use Exporter   ();
        our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
        $VERSION = sprintf "%d.%03d", q$Revision: 1.1 $ =~ /(\d+)/g;
        @ISA         = qw(Exporter);
        @EXPORT      = qw();
        %EXPORT_TAGS = ();
        @EXPORT_OK   = qw();
}
our @EXPORT_OK;

use constant MSG_REQ => 0x0001;
use constant MSG_TLV_RESV_INFO => 0x0011;
use constant MSG_TLV_PATH_ELEM => 0x0012;


die 'Abort: env variable $OSCARS_DIST undefined...' unless (defined($ENV{OSCARS_DIST}));

my $oscars_cmd =  "cd $ENV{OSCARS_DIST}/api; . bin/setclasspath.sh; java net.es.oscars.api.test.IDCTest -v 0.6 -a x509 -c list";
my $interval = 120; 

my $api_conn = new APIClient('localhost', '2091', 10101);

while(1) {
	my $resv_list_text = `$oscars_cmd`;
	my @resvs;
	#TODO: parse text into @resvs (each a \%resv={gri, status, start, end, bw, \@path}
	foreach my $resv (@resvs) {
		my $resv_info_tlv = new TLV(MSG_TLV_RESV_INFO, $resv->{gri}, $resv->{start}, $resv->{end}, $resv->{bw}, $resv->{status}); 
		my @path_elem_tlvs;
		foreach my $elem ($resv->{path}) {
			my $elem_tlv = new tlV(MSG_TLV_PATH_ELEM, $elem->{urn}, $elem->{swtype}, $elem->{enc}, $elem->{vlan});
			push(@path_elem_tlvs, $elem_tlv);
		}
                $api_conn->queue_bin_msg(MSG_REQ, 0, $resv_info_tlv, @path_elem_tlvs);
                $api_conn->send_bin_msg();
	}
	sleep($interval);
}

1;
