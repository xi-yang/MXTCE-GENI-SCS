#

package TLV;

use strict;
use warnings;

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
        # type(16) length(16) gri(8*64) start_time(32) end_time(32) bandwidth(32) status(8*16)
        if($a[0] == MSG_TLV_RESV_INFO) {
                $$self{templ} .= 'C64NNNC16';
                $$self{gri} = $a[1];
                $$self{start_time} = $a[2];
                $$self{end_time} = $a[3];
                $$self{bw} = $a[4];
                $$self{status} = $a[5];
        }
        elsif($a[0] == MSG_TLV_PATH_ELEM) {
                $$self{templ} .= 'C128CCn';
                $$self{urn} = $a[1];
                $$self{sw_type} = $a[2];
                $$self{enc_type} = $a[3];
                $$self{vlan} = $a[4];
        }
        # NOTE: this will calculate the correct length of the TLV as long as
        # the template consists of the number templates only (in the multiple of 8-bit
        # sizes) i.e. 'n', 'f', 'V13' will work,  'b', 'h', 'a*' etc. will not.
        my $templ = $$self{templ};
        $templ =~ s/(\D\d*)/$1 /g;
        chop $templ;
        my @l = split(/ /, $templ);
        # NOTE: this will be the length as entered to the TLV len field (TLV size minus four bytes of the header)
        $$self{len} = -(TLV_HDR_SIZE);
        for(my $i=0; $i<@l; $i++) {
                $$self{len} += length(pack($l[$i], 0));
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
                return pack($$self{templ}, $$self{type}, $$self{len}, $$self{gri}, $$self{start_time},
                        $$self{end_time}, $$self{bw}, $$self{status});
        }
        elsif($$self{type} == MSG_TLV_PATH_ELEM) {
                return pack($$self{templ}, $$self{type}, $$self{len}, $$self{urn}, $$self{sw_type}, $$self{enc_type},
			$$self{vlan});
        }
}

1;


