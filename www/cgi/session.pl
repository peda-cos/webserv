#!/usr/bin/perl
use strict;
use warnings;
use File::Path qw(make_path);
use File::Spec;

# 1. Configuração do diretório de sessões
my $session_dir = "/tmp/webserv_sessions";
unless (-d $session_dir) {
    make_path($session_dir);
}

# 2. Parse do Cookie
my $cookie_header = $ENV{'HTTP_COOKIE'} || "";
my %cookies;
foreach (split /;/, $cookie_header) {
    my ($k, $v) = split /=/, $_, 2;
    if (defined $k && defined $v) {
        $k =~ s/^\s+|\s+$//g;
        $cookies{$k} = $v;
    }
}

my $session_id = $cookies{'session_id'};
my $visits = 0;

# 3. Validação/Criação da Sessão
if ($session_id && $session_id =~ /^[a-zA-Z0-9\-]+$/) {
    my $session_file = File::Spec->catfile($session_dir, "session_$session_id");
    if (-e $session_file) {
        if (open(my $fh, '<', $session_file)) {
            $visits = <$fh>;
            chomp($visits);
            close($fh);
            $visits = int($visits || 0);
        }
    } else {
        $session_id = generate_uuid();
        $visits = 0;
    }
} else {
    $session_id = generate_uuid();
    $visits = 0;
}

$visits++;

# 4. Salvar sessão
my $session_file = File::Spec->catfile($session_dir, "session_$session_id");
if (open(my $fh, '>', $session_file)) {
    print $fh $visits;
    close($fh);
}

# 5. Output HTTP Headers
print "Content-Type: text/html\r\n";
print "Set-Cookie: session_id=$session_id; Path=/; HttpOnly\r\n";
print "\r\n";

# 6. Output Body
print <<"HTML";
<html>
<head><title>Webserv Session Test (Perl)</title></head>
<body>
    <h1>Session ID: $session_id</h1>
    <h2>Visit Count: $visits</h2>
    <p>Script executado via CGI Perl!</p>
    <p>Refresh para incrementar o contador.</p>
    <p><a href='/'>Voltar para Home</a></p>
</body>
</html>
HTML

# Função simples para gerar um ID aleatório
sub generate_uuid {
    my @chars = ('a'..'z', 'A'..'Z', 0..9);
    my $uuid = "";
    foreach (1..16) { $uuid .= $chars[rand @chars]; }
    return $uuid;
}
