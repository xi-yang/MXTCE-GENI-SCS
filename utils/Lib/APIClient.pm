#

package APIClient;

use strict;
use warnings;
use Socket;
use IO::Socket::INET;
use Errno;
use Lib::TLV;


BEGIN {
        use Exporter   ();
        our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
        $VERSION = sprintf "%d.%03d", q$Revision: 1.33 $ =~ /(\d+)/g;
        @ISA         = qw(Exporter);
        @EXPORT      = qw();
        %EXPORT_TAGS = ( );
        @EXPORT_OK   = qw();
}
our @EXPORT_OK;


sub new {
	shift;
        my ($host, $port, $ucid) = @_;
        $ucid = 1001 unless (defined($ucid));
        my $self;
        eval {
                $self = {
                        'server_host' => $host,
                        'server_port' => $port,
                        'bin_queue' => { 'fh' => undef, 'ucid' => $ucid,
                                'in' => {
                                        'seqn' => 0,
                                        'hdr' => undef,
                                        'data' => undef,
                                },
                                'out' => {
                                        'seqn' => 0,
                                        'hdr' => undef,
                                        'data' => ''
                                }
                        },
                };
        };
        bless $self;
        return $self;
}


sub chksum($$@) {
        my ($ppat, $upat, @d) = @_;
        my $block = pack($ppat, @d);
        my $chksum = unpack($upat, $block);
        return unpack("N", pack("V", $chksum));
}

sub connect_server() {
        my $self = shift;
        my $sock = IO::Socket::INET->new(
                PeerAddr => $self->{server_host},
                PeerPort => $self->{server_port},
                Proto     => 'tcp') or die 'API connect to '.$self->{server_host}.':'.$self->{server_port}.": $@\n";
        if($sock) {
                if($sock->connected()) {
                        $$self{bin_queue}{fh} = $sock;
                } 
        }
        return $sock;
}

sub disconnect_server() {
        my $self = shift;
        my $sock = $$self{bin_queue}{fh}; 
	if (defined($sock)) {
		$sock->shutdown();
	}
}

sub check_socket(;$) {
        my $self = shift;
        my $num_tries = shift;
        $num_tries = 1 unless (!defined($num_tries) || $num_tries < 1);
        my $sock = $$self{bin_queue}{fh};
        while ($num_tries > 0 && !$sock->connected()) {
            $sock = connect_server();
            if ($sock->connected()) {
                $$self{bin_queue}{fh} = $sock;
                return 1;
            }
            $num_tries--;
            sleep(1) if ($num_tries > 0);
        }
        return 0;
}

sub queue_bin_msg($$;$@) {
        my ($self, $type, $tag1, @data) = @_;
        my ($len, $ucid, $sn);
        my $dq = "";

        $ucid = $$self{bin_queue}{ucid};
        $sn = $$self{bin_queue}{out}{seqn}++;

        # process data (TLVs) if any
        $len = 0;
        if(@data) {
                for(my $i=0; $i<@data; $i++) {
                        $dq .= $data[$i]->get_bin();
                        $len += $data[$i]->get_len();
                }
        }
        $$self{bin_queue}{out}{data} .= $dq;
        my $chksum = chksum("nnNN", "%32V3", $type, $len, $ucid, $sn);
        printf("type =0x%x, len =0x%x, ucid = 0x%x, sn= 0x%x, chksum=0x%x\n", $type, $len, $ucid, $sn, $chksum);
        $$self{bin_queue}{out}{hdr} = {
                "type" => $type,
                "length" => $len,
                "ucid" => $ucid,
                "seqn" => $sn,
                "chksum" => $chksum,
                "tag1" => defined($tag1)?$tag1:0,
                "tag2" => 0
        };
}

sub send_bin_msg($) {
        my ($self) = @_;
        my $n = 0;
        my $bin;

        $bin = pack("nnNNNNN",
                $$self{bin_queue}{out}{hdr}{type},
                $$self{bin_queue}{out}{hdr}{length},
                $$self{bin_queue}{out}{hdr}{ucid},
                $$self{bin_queue}{out}{hdr}{seqn},
                $$self{bin_queue}{out}{hdr}{chksum},
                $$self{bin_queue}{out}{hdr}{tag1},
                $$self{bin_queue}{out}{hdr}{tag2}
        );

        $bin .= $$self{bin_queue}{out}{data};
        $n = $$self{bin_queue}{fh}->syswrite($bin);
        if(defined($n)) {
                $bin = substr($bin, $n);
                $$self{bin_queue}{out}{data} = '';
        }
        else {
                $$self{bin_queue}{out}{data} = '';
        }
}

sub get_bin_msg($) {
        my ($self) = @_;
        my $hdr;
        my $data;
        my ($type, $len, $ucid, $sn, $chksum, $tag1, $tag2);

        if(!defined($$self{bin_queue}{fh}->connected())) {
                die "client not connected\n";
        }
        my $n = $$self{bin_queue}{fh}->sysread($hdr, 24);
        if(!defined($n)) {
                if($! != Errno::EINTR) {
                        die "socket error\n";
                }
                die "connection terminated\n";
        }
        if(!$n) {
                die "client disconnect\n";
        }
        if($n != 24) {
                die "APIClient: malformed msg. header (wrong length)\n";
        }
        ($type, $len, $ucid, $sn, $chksum, $tag1, $tag2) = unpack("nnNNNNN", $hdr);
        if(!(
                        defined($type) &&
                        defined($len) &&
                        defined($ucid) &&
                        defined($sn) &&
                        defined($chksum) &&
                        defined($tag1) &&
                        defined($tag2))) {
                die "APIClient: malformed msg. header\n";
        }
        $$self{bin_queue}{in}{hdr} = {
                "type" => $type,
                "length" => $len,
                "ucid" => $ucid,
                "seqn" => $sn,
                "chksum" => $chksum,
                "tag1" => $tag1,
                "tag2" => $tag2
        };
        $$self{bin_queue}{in}{data} = undef;
        # and the body
        if($len > 0) {
                $n = $$self{bin_queue}{fh}->sysread($data, $len);
                if(!defined($n)) {
                        die "socket error\n";
                }
                $$self{bin_queue}{in}{data} = $data;
        }
}

1;

