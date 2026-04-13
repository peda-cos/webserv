#!/usr/bin/perl
use strict;
use warnings;

print "Content-Type: text/html\r\n\r\n";
print "<html><body><h1>Perl CGI Environment</h1>";

my @required_vars = (
    'CONTENT_LENGTH',
    'CONTENT_TYPE',
    'GATEWAY_INTERFACE',
    'PATH_INFO',
    'QUERY_STRING',
    'REQUEST_METHOD',
    'REQUEST_URI',
    'SCRIPT_NAME',
    'SERVER_PROTOCOL',
    'SERVER_SOFTWARE'
);

for my $key (@required_vars) {
    my $value = exists $ENV{$key} ? $ENV{$key} : '';
    $value =~ s/&/&amp;/g;
    $value =~ s/</&lt;/g;
    $value =~ s/>/&gt;/g;
    print $key . ": " . $value . "<br>";
}

if ((exists $ENV{'REQUEST_METHOD'} ? $ENV{'REQUEST_METHOD'} : '') eq 'POST') {
    my $len = exists $ENV{'CONTENT_LENGTH'} ? int($ENV{'CONTENT_LENGTH'}) : 0;
    my $body = '';
    if ($len > 0) {
        read(STDIN, $body, $len);
    }
    $body =~ s/&/&amp;/g;
    $body =~ s/</&lt;/g;
    $body =~ s/>/&gt;/g;
    print "<hr>POST body: " . $body . "<br>";
}

print "</body></html>";
