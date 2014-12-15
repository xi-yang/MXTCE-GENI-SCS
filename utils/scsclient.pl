#!/usr/bin/perl
use strict;
my $file = shift(@ARGV);
my $scs = shift(@ARGV);

print $file;

my $rspec = do {
    local $/ = undef;
    open my $fh, "<", $file
        or die "could not open $file: $!";
    <$fh>;
};

$rspec =~ s/\</\&lt\;/g;
$rspec =~ s/\>/\&gt\;/g;

my $scs_request = "<methodCall>
  <methodName>ComputePath</methodName>
  <params>
    <param>
      <value>
        <struct>
          <member>
            <name>slice_urn</name>
            <value>
              <string>urn:publicid:IDN+pgeni.gpolab.bbn.com+slice+ahtest</string>
            </value>
          </member>
          <member>
            <name>request_rspec</name>
              <value>
                <string>_replace_with_rspec_</string>
               </value>
             </member>
             <member>
               <name>request_options</name>
               <value>
                 <struct/>
               </value>
             </member>
           </struct>
         </value>
       </param>
     </params>
</methodCall>";

$scs_request =~ s/_replace_with_rspec_/$rspec/g;

print $scs;

unless ($scs) {
    print $scs_request;
    exit;
}

my $scs_req_file = "/tmp/scs-req.xml";
open my $fh, ">", $scs_req_file
      or die "could not open $scs_req_file: $!";
print $fh $scs_request;

my $scs_output = `cat $scs_req_file | curl -X POST -H 'Content-type: text/xml' -d \@- http://$scs:8081/geni/xmlrpc`;
print $scs_output;
