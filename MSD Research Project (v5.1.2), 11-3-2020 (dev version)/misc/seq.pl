my $N = 35; #const
my $k = 2; #const

my @seq;
for( my $n = -$N; $n <= $N; $n++ ) {
	push (@seq, ($k / $n)) if $n != 0;
}

print "{";
for (sort {$a <=> $b} @seq) {
	print " $_ ";
}
print "}\n";