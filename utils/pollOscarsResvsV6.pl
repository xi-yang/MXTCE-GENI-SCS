#!/usr/bin/perl

use strict;
use warnings;
use Lib::TLV;
use Lib::APIClient;
use DBI;

use constant API_MSG_RESV_PUSH => 0x0011;
use constant MSG_TLV_RESV_INFO => 0x0011;
use constant MSG_TLV_PATH_ELEM => 0x0012;

my $host = shift;
my $domain = shift;
my $interval = shift;

$host = 'localhost' unless defined($host);
$domain = '' unless defined($domain);
$interval = 60 unless defined($interval);


my $dbh = DBI->connect('dbi:mysql:rm','oscars','mypass') or die "Connection Error: $DBI::errstr\n";

my $sql = "select R.globalReservationId,R.status,R.startTime,R.endTime,R.bandwidth,E.urn,V.swcap,V.value from reservations as R,paths as P,stdConstraints as S,pathElems as E,pathElemParams as V where (R.status='ACTIVE' or R.status='RESERVED' or R.status='INSETUP') and P.pathType='strict' and V.type='suggestedVlan' and S.reservationId=R.id and S.pathId=P.id and E.pathId=P.id and V.pathElemId=E.id order by R.id, E.id";

my $api_conn = new APIClient($host, '2091', 10101);
$api_conn->connect_server();

sub parse_resvs($)
{
    my $sth = shift;
    my @resvs;
    while (my @row = $sth->fetchrow_array) {
        my $resv = $resvs[-1];
        if (!defined($resv) || $resv->{gri} ne $row[0]) {
            my %resvMem;
            $resv = \%resvMem;
            $resv->{gri} = $row[0];
            $resv->{status} = $row[1];
            $resv->{start} = $row[2];
            $resv->{end} = $row[3];
            $resv->{bw} = $row[4];
            push(@resvs, $resv);
        }
        if (!defined($resv->{path})) {
            my @path;
            $resv->{path} = \@path;
        }

        next if (defined($resv->{path}[-1]) && $resv->{path}[-1]{urn} eq $row[5]); 

        my %elemMem;
        my $elem = \%elemMem;
        $elem->{urn} = $row[5];
        $elem->{swtype} = 51;
        $elem->{enc} = 2;
        $elem->{vlan} = $row[7];
        if ($row[7] eq "null") {
            $elem->{vlan} = 0;
        }
        push(@{$resv->{path}}, $elem);
    }
    return @resvs;
}

while(1) {
    unless ($api_conn->check_socket()) {
        sleep(30);
        print "\n\tAPI socket broken...retry in 30 seconds\n";
        next;
    }
    my $sth = $dbh->prepare($sql);
    $sth->execute or die "SQL Error: $DBI::errstr\n";
    my @resvs = parse_resvs($sth);
    my $ct = 1;
    foreach my $resv (@resvs) {
        my $resv_info_tlv = new TLV(MSG_TLV_RESV_INFO, $resv->{gri}, $domain, $resv->{start}, $resv->{end}, $resv->{bw}, $resv->{status});
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
        if ($ct == $#resvs+1) {
                $opts = ($opts | 0x0002);
        }
        $api_conn->queue_bin_msg(API_MSG_RESV_PUSH, $opts, $resv_info_tlv, @path_elem_tlvs);
        $ct++;
        $api_conn->send_bin_msg();
        print %{$resv};
        print "\n";
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
