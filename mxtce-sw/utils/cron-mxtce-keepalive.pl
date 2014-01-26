#!/usr/bin/perl
my $output = `ps aux | grep mxtce | grep -v grep`;
unless ($output =~ /mxtce -d/) {
        `sleep 1; cd /usr/local/mxtce-sw/resources/topology/; /usr/local/mxtce-sw/src/main/mxtce -d`;
        $datestring = localtime();
        print "\n!!! MXTCE restarted at $datestring !!!\n";
}

