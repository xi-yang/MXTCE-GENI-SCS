#

package TLV;

use strict;
use warnings;

use Math::BigInt;

BEGIN {
        use Exporter   ();
        our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
        $VERSION = sprintf "%d.%03d", q$Revision: 1.1 $ =~ /(\d+)/g;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(MSG_TLV_RESV_INFO MSG_TLV_PATH_ELEM);
        %EXPORT_TAGS = ();
        @EXPORT_OK   = qw();
}
our @EXPORT_OK;

use constant TLV_HDR_SIZE => 4;
use constant MSG_TLV_RESV_INFO => 0x0011;
use constant MSG_TLV_PATH_ELEM => 0x0012;

sub new {
        shift;
        my (@a) = @_;
        my $self = {
                'type' => $a[0],
                'len' => undef,
                'templ' => 'nn' #the header
        };
        # type(16) length(16) gri(8*64) domain(8*36) start_time(32) end_time(32) bandwidth(64) status(8*16)
        if($a[0] == MSG_TLV_RESV_INFO) {
                $$self{templ} .= 'a64a36LLLLa16';
                $$self{len} = 132;
                $$self{gri} = $a[1];
                $$self{domain} = $a[2];
                $$self{start_time} = $a[3];
                $$self{end_time} = $a[4];
                $$self{bw} = $a[5];
                $$self{status} = $a[6];
        }
        elsif($a[0] == MSG_TLV_PATH_ELEM) {
                $$self{templ} .= 'a128CCv';
                $$self{len} = 132;
                $$self{urn} = $a[1];
                $$self{sw_type} = $a[2];
                $$self{enc_type} = $a[3];
                $$self{vlan} = $a[4];
        }
        bless $self;
        return $self;
}

sub get_len() {
        my $self = shift;
        return $$self{len} + TLV_HDR_SIZE;
}

sub get_bin() {
        my $self = shift;
        if($$self{type} == MSG_TLV_RESV_INFO) {
                my $bw = Math::BigInt->new($$self{bw});
                my($bw1,$bw2) = (int($bw%2**32), int($bw/2**32)%2**32);
                print "bw1=$bw1, bw2=$bw2\n";
                return pack($$self{templ}, $$self{type}, $$self{len}, $$self{gri}, $$self{domain}, $$self{start_time},
                        $$self{end_time}, $bw1, $bw2, $$self{status});
        }
        elsif($$self{type} == MSG_TLV_PATH_ELEM) {
                return pack($$self{templ}, $$self{type}, $$self{len}, $$self{urn}, $$self{sw_type}, $$self{enc_type},
			$$self{vlan});
        }
}

1;


