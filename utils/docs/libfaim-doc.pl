#!/usr/bin/perl

## Copyright (c) 1998 Michael Zucchi, All Rights Reserved        ##
## Copyright (C) 2000  Tim Waugh <twaugh@redhat.com>             ##
##                                                               ##
## This software falls under the GNU General Public License.     ##
## Please read the COPYING file for more information             ##

#
# This will read a 'c' file and scan for embedded comments in the
# style of gnome comments (+minor extensions - see below).
#

# Note: This only supports 'c'.

# usage:
# kerneldoc [ -docbook | -html | -text | -man ]
#           [ -function funcname [ -function funcname ...] ] c file(s)s > outputfile
# or
#           [ -nofunction funcname [ -function funcname ...] ] c file(s)s > outputfile
#
#  Set output format using one of -docbook -html -text or -man.  Default is man.
#
#  -function funcname
#	If set, then only generate documentation for the given function(s).  All
#	other functions are ignored.
#
#  -nofunction funcname
#	If set, then only generate documentation for the other function(s).  All
#	other functions are ignored. Cannot be used with -function together
#	(yes thats a bug - perl hackers can fix it 8))
#
#  c files - list of 'c' files to process
#
#  All output goes to stdout, with errors to stderr.

#
# format of comments.
# In the following table, (...)? signifies optional structure.
#                         (...)* signifies 0 or more structure elements
# /**
#  * function_name(:)? (- short description)?
# (* @parameterx: (description of parameter x)?)*
# (* a blank line)?
#  * (Description:)? (Description of function)?
#  * (section header: (section description)? )*
#  (*)?*/
#
# So .. the trivial example would be:
#
# /**
#  * my_function
#  **/
#
# If the Description: header tag is ommitted, then there must be a blank line
# after the last parameter specification.
# e.g.
# /**
#  * my_function - does my stuff
#  * @my_arg: its mine damnit
#  *
#  * Does my stuff explained. 
#  */
#
#  or, could also use:
# /**
#  * my_function - does my stuff
#  * @my_arg: its mine damnit
#  * Description: Does my stuff explained. 
#  */
# etc.
#
# All descriptions can be multiline, apart from the short function description.
#
# All descriptive text is further processed, scanning for the following special
# patterns, which are highlighted appropriately.
#
# 'funcname()' - function
# '$ENVVAR' - environmental variable
# '&struct_name' - name of a structure (up to two words including 'struct')
# '@parameter' - name of a parameter
# '%CONST' - name of a constant.

# match expressions used to find embedded type information
$type_constant = "\\\%([-_\\w]+)";
$type_func = "(\\w+)\\(\\)";
$type_param = "\\\@(\\w+)";
$type_struct = "\\\&((struct\\s*)?\\w+)";
$type_env = "(\\\$\\w+)";


# Output conversion substitutions.
#  One for each output format

# these work fairly well
%highlights_html = ( $type_constant, "<i>\$1</i>",
		     $type_func, "<b>\$1</b>",
		     $type_struct, "<i>\$1</i>",
		     $type_param, "<tt><b>\$1</b></tt>" );
$blankline_html = "<p>";

# sgml, docbook format
%highlights_sgml = ( "([^=])\\\"([^\\\"<]+)\\\"", "\$1<quote>\$2</quote>",
		     $type_constant, "<constant>\$1</constant>",
		     $type_func, "<function>\$1</function>",
		     $type_struct, "<structname>\$1</structname>",
		     $type_env, "<envar>\$1</envar>",
		     $type_param, "<parameter>\$1</parameter>" );
$blankline_sgml = "</para><para>\n";

# gnome, docbook format
%highlights_gnome = ( $type_constant, "<replaceable class=\"option\">\$1</replaceable>",
		     $type_func, "<function>\$1</function>",
		     $type_struct, "<structname>\$1</structname>",
		     $type_env, "<envar>\$1</envar>",
		     $type_param, "<parameter>\$1</parameter>" );
$blankline_gnome = "</para><para>\n";

# these are pretty rough
%highlights_man = ( $type_constant, "\$1",
		    $type_func, "\\\\fB\$1\\\\fP",
		    $type_struct, "\\\\fI\$1\\\\fP",
		    $type_param, "\\\\fI\$1\\\\fP" );
$blankline_man = "";

# text-mode
%highlights_text = ( $type_constant, "\$1",
		     $type_func, "\$1",
		     $type_struct, "\$1",
		     $type_param, "\$1" );
$blankline_text = "";


sub usage {
    print "Usage: $0 [ -v ] [ -docbook | -html | -text | -man ]\n";
    print "         [ -function funcname [ -function funcname ...] ]\n";
    print "         [ -nofunction funcname [ -nofunction funcname ...] ]\n";
    print "         c source file(s) > outputfile\n";
    exit 1;
}

# read arguments
if ($#ARGV==-1) {
    usage();
}

$verbose = 0;
$output_mode = "man";
%highlights = %highlights_man;
$blankline = $blankline_man;
$modulename = "libfaim API Documentation";
$function_only = 0;
while ($ARGV[0] =~ m/^-(.*)/) {
    $cmd = shift @ARGV;
    if ($cmd eq "-html") {
	$output_mode = "html";
	%highlights = %highlights_html;
	$blankline = $blankline_html;
    } elsif ($cmd eq "-man") {
	$output_mode = "man";
	%highlights = %highlights_man;
	$blankline = $blankline_man;
    } elsif ($cmd eq "-text") {
	$output_mode = "text";
	%highlights = %highlights_text;
	$blankline = $blankline_text;
    } elsif ($cmd eq "-docbook") {
	$output_mode = "sgml";
	%highlights = %highlights_sgml;
	$blankline = $blankline_sgml;
    } elsif ($cmd eq "-gnome") {
	$output_mode = "gnome";
	%highlights = %highlights_gnome;
	$blankline = $blankline_gnome;
    } elsif ($cmd eq "-module") { # not needed for sgml, inherits from calling document
	$modulename = shift @ARGV;
    } elsif ($cmd eq "-function") { # to only output specific functions
	$function_only = 1;
	$function = shift @ARGV;
	$function_table{$function} = 1;
    } elsif ($cmd eq "-nofunction") { # to only output specific functions
	$function_only = 2;
	$function = shift @ARGV;
	$function_table{$function} = 1;
    } elsif ($cmd eq "-v") {
	$verbose = 1;
    } elsif (($cmd eq "-h") || ($cmd eq "--help")) {
	usage();
    }
}


# generate a sequence of code that will splice in highlighting information
# using the s// operator.
$dohighlight = "";
foreach $pattern (keys %highlights) {
#    print "scanning pattern $pattern ($highlights{$pattern})\n";
    $dohighlight .=  "\$contents =~ s:$pattern:$highlights{$pattern}:gs;\n";
}

##
# dumps section contents to arrays/hashes intended for that purpose.
#
sub dump_section {
    my $name = shift @_;
    my $contents = join "\n", @_;

    if ($name =~ m/$type_constant/) {
	$name = $1;
#	print STDERR "constant section '$1' = '$contents'\n";
	$constants{$name} = $contents;
    } elsif ($name =~ m/$type_param/) {
#	print STDERR "parameter def '$1' = '$contents'\n";
	$name = $1;
	$parameters{$name} = $contents;
    } else {
#	print STDERR "other section '$name' = '$contents'\n";
	$sections{$name} = $contents;
	push @sectionlist, $name;
    }
}

##
# output function
#
# parameters, a hash.
#  function => "function name"
#  parameterlist => @list of parameters
#  parameters => %parameter descriptions
#  sectionlist => @list of sections
#  sections => %descriont descriptions
#  

sub output_highlight {
    my $contents = join "\n", @_;
    my $line;

    eval $dohighlight;
    foreach $line (split "\n", $contents) {
	if ($line eq ""){
	    print $lineprefix, $blankline;
	} else {
            $line =~ s/\\\\\\/\&/g;
	    print $lineprefix, $line;
	}
	print "\n";
    }
}


# output in html
sub output_html {
    my %args = %{$_[0]};
    my ($parameter, $section);
    my $count;
    print "<h2>Function</h2>\n";

    print "<i>".$args{'functiontype'}."</i>\n";
    print "<b>".$args{'function'}."</b>\n";
    print "(";
    $count = 0;
    foreach $parameter (@{$args{'parameterlist'}}) {
	$type = $args{'parametertypes'}{$parameter};
	if ($type =~ m/([^\(]*\(\*)\s*\)\s*\(([^\)]*)\)/) {
	    # pointer-to-function
	    print "<i>$1</i><b>$parameter</b>) <i>($2)</i>";
	} else {
	    print "<i>".$type."</i> <b>".$parameter."</b>";
	}
	if ($count != $#{$args{'parameterlist'}}) {
	    $count++;
	    print ",\n";
	}
    }
    print ")\n";

    print "<h3>Arguments</h3>\n";
    print "<dl>\n";
    foreach $parameter (@{$args{'parameterlist'}}) {
	print "<dt><b>".$parameter."</b>\n";
	print "<dd>";
	output_highlight($args{'parameters'}{$parameter});
    }
    print "</dl>\n";
    foreach $section (@{$args{'sectionlist'}}) {
	print "<h3>$section</h3>\n";
	print "<blockquote>\n";
	output_highlight($args{'sections'}{$section});
	print "</blockquote>\n";
    }
    print "<hr>\n";
}


# output in html
sub output_intro_html {
    my %args = %{$_[0]};
    my ($parameter, $section);
    my $count;

    foreach $section (@{$args{'sectionlist'}}) {
	print "<h3>$section</h3>\n";
	print "<ul>\n";
	output_highlight($args{'sections'}{$section});
	print "</ul>\n";
    }
    print "<hr>\n";
}



# output in sgml DocBook
sub output_sgml {
    my %args = %{$_[0]};
    my ($parameter, $section);
    my $count;
    my $id;

    $id = "API-".$args{'function'};
    $id =~ s/[^A-Za-z0-9]/-/g;

    print "<refentry>\n";
    print "<refmeta>\n";
    print "<refentrytitle><phrase id=\"$id\">".$args{'function'}."</phrase></refentrytitle>\n";
    print "</refmeta>\n";
    print "<refnamediv>\n";
    print " <refname>".$args{'function'}."</refname>\n";
    print " <refpurpose>\n";
    print "  ".$args{'purpose'}."\n";
    print " </refpurpose>\n";
    print "</refnamediv>\n";

    print "<refsynopsisdiv>\n";
    print " <title>Synopsis</title>\n";
    print "  <funcsynopsis>\n";
    print "   <funcdef>".$args{'functiontype'}." ";
    print "<function>".$args{'function'}." ";
    print "</function></funcdef>\n";

#    print "<refsect1>\n";
#    print " <title>Synopsis</title>\n";
#    print "  <funcsynopsis>\n";
#    print "   <funcdef>".$args{'functiontype'}." ";
#    print "<function>".$args{'function'}." ";
#    print "</function></funcdef>\n";

    $count = 0;
    if ($#{$args{'parameterlist'}} >= 0) {
	foreach $parameter (@{$args{'parameterlist'}}) {
	    $type = $args{'parametertypes'}{$parameter};
	    if ($type =~ m/([^\(]*\(\*)\s*\)\s*\(([^\)]*)\)/) {
		# pointer-to-function
		print "   <paramdef>$1<parameter>$parameter</parameter>)\n";
		print "     <funcparams>$2</funcparams></paramdef>\n";
	    } else {
		print "   <paramdef>".$type;
		print " <parameter>$parameter</parameter></paramdef>\n";
	    }
	}
    } else {
	print "  <void>\n";
    }
    print "  </funcsynopsis>\n";
    print "</refsynopsisdiv>\n";
#    print "</refsect1>\n";

    # print parameters
    print "<refsect1>\n <title>Arguments</title>\n";
#    print "<para>\nArguments\n";
    if ($#{$args{'parameterlist'}} >= 0) {
	print " <variablelist>\n";
	foreach $parameter (@{$args{'parameterlist'}}) {
	    print "  <varlistentry>\n   <term><parameter>$parameter</parameter></term>\n";
	    print "   <listitem>\n    <para>\n";
	    $lineprefix="     ";
	    output_highlight($args{'parameters'}{$parameter});
	    print "    </para>\n   </listitem>\n  </varlistentry>\n";
	}
	print " </variablelist>\n";
    } else {
	print " <para>\n  None\n </para>\n";
    }
    print "</refsect1>\n";

    # print out each section
    $lineprefix="   ";
    foreach $section (@{$args{'sectionlist'}}) {
	print "<refsect1>\n <title>$section</title>\n <para>\n";
#	print "<para>\n$section\n";
	if ($section =~ m/EXAMPLE/i) {
	    print "<example><para>\n";
	}
	output_highlight($args{'sections'}{$section});
#	print "</para>";
	if ($section =~ m/EXAMPLE/i) {
	    print "</para></example>\n";
	}
	print " </para>\n</refsect1>\n";
    }

    print "</refentry>\n\n";
}

# output in sgml DocBook
sub output_intro_sgml {
    my %args = %{$_[0]};
    my ($parameter, $section);
    my $count;
    my $id;

    $id = $args{'module'};
    $id =~ s/[^A-Za-z0-9]/-/g;

    # print out each section
    $lineprefix="   ";
    foreach $section (@{$args{'sectionlist'}}) {
	print "<refsect1>\n <title>$section</title>\n <para>\n";
#	print "<para>\n$section\n";
	if ($section =~ m/EXAMPLE/i) {
	    print "<example><para>\n";
	}
	output_highlight($args{'sections'}{$section});
#	print "</para>";
	if ($section =~ m/EXAMPLE/i) {
	    print "</para></example>\n";
	}
	print " </para>\n</refsect1>\n";
    }

    print "\n\n";
}

# output in sgml DocBook
sub output_gnome {
    my %args = %{$_[0]};
    my ($parameter, $section);
    my $count;
    my $id;

    $id = $args{'module'}."-".$args{'function'};
    $id =~ s/[^A-Za-z0-9]/-/g;

    print "<sect2>\n";
    print " <title id=\"$id\">".$args{'function'}."</title>\n";

#    print "<simplesect>\n";
#    print " <title>Synopsis</title>\n";
    print "  <funcsynopsis>\n";
    print "   <funcdef>".$args{'functiontype'}." ";
    print "<function>".$args{'function'}." ";
    print "</function></funcdef>\n";

    $count = 0;
    if ($#{$args{'parameterlist'}} >= 0) {
	foreach $parameter (@{$args{'parameterlist'}}) {
	    $type = $args{'parametertypes'}{$parameter};
	    if ($type =~ m/([^\(]*\(\*)\s*\)\s*\(([^\)]*)\)/) {
		# pointer-to-function
		print "   <paramdef>$1 <parameter>$parameter</parameter>)\n";
		print "     <funcparams>$2</funcparams></paramdef>\n";
	    } else {
		print "   <paramdef>".$type;
		print " <parameter>$parameter</parameter></paramdef>\n";
	    }
	}
    } else {
	print "  <void>\n";
    }
    print "  </funcsynopsis>\n";
#    print "</simplesect>\n";
#    print "</refsect1>\n";

    # print parameters
#    print "<simplesect>\n <title>Arguments</title>\n";
#    if ($#{$args{'parameterlist'}} >= 0) {
#	print " <variablelist>\n";
#	foreach $parameter (@{$args{'parameterlist'}}) {
#	    print "  <varlistentry>\n   <term><parameter>$parameter</parameter></term>\n";
#	    print "   <listitem>\n    <para>\n";
#	    $lineprefix="     ";
#	    output_highlight($args{'parameters'}{$parameter});
#	    print "    </para>\n   </listitem>\n  </varlistentry>\n";
#	}
#	print " </variablelist>\n";
#    } else {
#	print " <para>\n  None\n </para>\n";
#    }
#    print "</simplesect>\n";

#    print "<simplesect>\n <title>Arguments</title>\n";
    if ($#{$args{'parameterlist'}} >= 0) {
	print " <informaltable pgwide=\"1\" frame=\"none\" role=\"params\">\n";
	print "<tgroup cols=\"2\">\n";
	print "<colspec colwidth=\"2*\">\n";
	print "<colspec colwidth=\"8*\">\n";
	print "<tbody>\n";
	foreach $parameter (@{$args{'parameterlist'}}) {
	    print "  <row><entry align=\"right\"><parameter>$parameter</parameter></entry>\n";
	    print "   <entry>\n";
	    $lineprefix="     ";
	    output_highlight($args{'parameters'}{$parameter});
	    print "    </entry></row>\n";
	}
	print " </tbody></tgroup></informaltable>\n";
    } else {
	print " <para>\n  None\n </para>\n";
    }
#    print "</simplesect>\n";

    # print out each section
    $lineprefix="   ";
    foreach $section (@{$args{'sectionlist'}}) {
	print "<simplesect>\n <title>$section</title>\n";
#	print "<para>\n$section\n";
	if ($section =~ m/EXAMPLE/i) {
	    print "<example><programlisting>\n";
	} else {
	}
	print "<para>\n";
	output_highlight($args{'sections'}{$section});
#	print "</para>";
	print "</para>\n";
	if ($section =~ m/EXAMPLE/i) {
	    print "</programlisting></example>\n";
	} else {
	}
	print " </simplesect>\n";
    }

    print "</sect2>\n\n";
}

##
# output in man
sub output_man {
    my %args = %{$_[0]};
    my ($parameter, $section);
    my $count;

    print ".TH \"$args{'module'}\" 3 \"$args{'function'}\" \"25 May 1998\" \"API Manual\" libfaim\n";

    print ".SH NAME\n";
    print $args{'function'}." \\- ".$args{'purpose'}."\n";

    print ".SH SYNOPSIS\n";
    print ".B \"".$args{'functiontype'}."\" ".$args{'function'}."\n";
    $count = 0;
    $parenth = "(";
    $post = ",";
    foreach $parameter (@{$args{'parameterlist'}}) {
	if ($count == $#{$args{'parameterlist'}}) {
	    $post = ");";
	}
	$type = $args{'parametertypes'}{$parameter};
	if ($type =~ m/([^\(]*\(\*)\s*\)\s*\(([^\)]*)\)/) {
	    # pointer-to-function
	    print ".BI \"".$parenth.$1."\" ".$parameter." \") (".$2.")".$post."\"\n";
	} else {
	    $type =~ s/([^\*])$/\1 /;
	    print ".BI \"".$parenth.$type."\" ".$parameter." \"".$post."\"\n";
	}
	$count++;
	$parenth = "";
    }

    print ".SH Arguments\n";
    foreach $parameter (@{$args{'parameterlist'}}) {
	print ".IP \"".$parameter."\" 12\n";
	output_highlight($args{'parameters'}{$parameter});
    }
    foreach $section (@{$args{'sectionlist'}}) {
	print ".SH \"$section\"\n";
	output_highlight($args{'sections'}{$section});
    }
}

sub output_intro_man {
    my %args = %{$_[0]};
    my ($parameter, $section);
    my $count;

    print ".TH \"$args{'module'}\" 3 \"$args{'module'}\" \"25 May 1998\" \"API Manual\" libfaim\n";

    foreach $section (@{$args{'sectionlist'}}) {
	print ".SH \"$section\"\n";
	output_highlight($args{'sections'}{$section});
    }
}

##
# output in text
sub output_text {
    my %args = %{$_[0]};
    my ($parameter, $section);

    print "Function:\n\n";
    $start=$args{'functiontype'}." ".$args{'function'}." (";
    print $start;
    $count = 0;
    foreach $parameter (@{$args{'parameterlist'}}) {
	if ($type =~ m/([^\(]*\(\*)\s*\)\s*\(([^\)]*)\)/) {
	    # pointer-to-function
	    print $1.$parameter.") (".$2;
	} else {
	    print $type." ".$parameter;
	}
	if ($count != $#{$args{'parameterlist'}}) {
	    $count++;
	    print ",\n";
	    print " " x length($start);
	} else {
	    print ");\n\n";
	}
    }

    print "Arguments:\n\n";
    foreach $parameter (@{$args{'parameterlist'}}) {
	print $parameter."\n\t".$args{'parameters'}{$parameter}."\n";
    }
    foreach $section (@{$args{'sectionlist'}}) {
	print "$section:\n\n";
	output_highlight($args{'sections'}{$section});
    }
    print "\n\n";
}

sub output_intro_text {
    my %args = %{$_[0]};
    my ($parameter, $section);

    foreach $section (@{$args{'sectionlist'}}) {
	print " $section:\n";
	print "    -> ";
	output_highlight($args{'sections'}{$section});
    }
}

##
# generic output function - calls the right one based
# on current output mode.
sub output_function {
#    output_html(@_);
    eval "output_".$output_mode."(\@_);";
}

##
# generic output function - calls the right one based
# on current output mode.
sub output_intro {
#    output_html(@_);
    eval "output_intro_".$output_mode."(\@_);";
}


##
# takes a function prototype and spits out all the details
# stored in the global arrays/hsahes.
sub dump_function {
    my $prototype = shift @_;

    $prototype =~ s/^static+ //;
    $prototype =~ s/^extern+ //;
    $prototype =~ s/^inline+ //;
    $prototype =~ s/^__inline__+ //;
    $prototype =~ s/^faim_export+ //;
    $prototype =~ s/^faim_internal+ //;   

    if ($prototype =~ m/^()([a-zA-Z0-9_~:]+)\s*\(([^\{]*)\)/ ||
	$prototype =~ m/^(\w+)\s+([a-zA-Z0-9_~:]+)\s*\(([^\{]*)\)/ ||
	$prototype =~ m/^(\w+\s*\*)\s*([a-zA-Z0-9_~:]+)\s*\(([^\{]*)\)/ ||
	$prototype =~ m/^(\w+\s+\w+)\s+([a-zA-Z0-9_~:]+)\s*\(([^\{]*)\)/ ||
	$prototype =~ m/^(\w+\s+\w+\s*\*)\s*([a-zA-Z0-9_~:]+)\s*\(([^\{]*)\)/)  {
	$return_type = $1;
	$function_name = $2;
	$args = $3;

	# allow for up to fours args to function pointers
	$args =~ s/(\([^\),]+),/\1#/g;
	$args =~ s/(\([^\),]+),/\1#/g;
	$args =~ s/(\([^\),]+),/\1#/g;
#	print STDERR "ARGS = '$args'\n";

	foreach $arg (split ',', $args) {
	    # strip leading/trailing spaces
	    $arg =~ s/^\s*//;
	    $arg =~ s/\s*$//;

	    if ($arg =~ m/\(/) {
		# pointer-to-function
		$arg =~ tr/#/,/;
		$arg =~ m/[^\(]+\(\*([^\)]+)\)/;
		$param = $1;
		$type = $arg;
		$type =~ s/([^\(]+\(\*)$param/\1/;
	    } else {
		# evil magic to get fixed array parameters to work
		$arg =~ s/(.+\s+)(.+)\[.*/\1* \2/;
#		print STDERR "SCAN ARG: '$arg'\n";
		@args = split('\s', $arg);

#		print STDERR " -> @args\n";
		$param = pop @args;
#		print STDERR " -> @args\n";
		if ($param =~ m/^(\*+)(.*)/) {
		    $param = $2;
		    push @args, $1;
		}
		$type = join " ", @args;
	    }

	    if ($type eq "" && $param eq "...")
	    {
		$type="...";
		$param="...";
		$parameters{"..."} = "variable arguments";
	    }
	    if ($type eq "")
	    {
		$type="";
		$param="void";
		$parameters{void} = "no arguments";
	    }
            if ($parameters{$param} eq "") {
	        $parameters{$param} = "-- undescribed --";
	        print STDERR "Warning($file:$lineno): Function parameter '$param' not described in '$function_name'\n";
	    }

	    push @parameterlist, $param;
	    $parametertypes{$param} = $type;
#	    print STDERR "param = '$param', type = '$type'\n";
	}
    } else {
	print STDERR "Error($lineno): cannot understand prototype: '$prototype'\n";
	return;
    }

    if ($function_only==0 || 
     ( $function_only == 1 && defined($function_table{$function_name})) || 
     ( $function_only == 2 && !defined($function_table{$function_name})))
    {
	output_function({'function' => $function_name,
			 'module' => $modulename,
			 'functiontype' => $return_type,
			 'parameterlist' => \@parameterlist,
			 'parameters' => \%parameters,
			 'parametertypes' => \%parametertypes,
			 'sectionlist' => \@sectionlist,
			 'sections' => \%sections,
			 'purpose' => $function_purpose
			 });
    }
}

######################################################################
# main
# states
# 0 - normal code
# 1 - looking for function name
# 2 - scanning field start.
# 3 - scanning prototype.
$state = 0;
$section = "";

$doc_special = "\@\%\$\&";

$doc_start = "^/\\*\\*\$";
$doc_end = "\\*/";
$doc_com = "\\s*\\*\\s*";
$doc_func = $doc_com."(\\w+):?";
$doc_sect = $doc_com."([".$doc_special."]?[\\w ]+):(.*)";
$doc_content = $doc_com."(.*)";
$doc_block = $doc_com."DOC:\\s*(.*)?";

%constants = ();
%parameters = ();
@parameterlist = ();
%sections = ();
@sectionlist = ();

$contents = "";
$section_default = "Description";	# default section
$section_intro = "Introduction";
$section = $section_default;

$lineno = 0;
foreach $file (@ARGV) {
    chomp($file);
    if (!open(IN,"<$file")) {
	print STDERR "Error: Cannot open file $file\n";
	next;
    }
    while (<IN>) {
	$lineno++;

	if ($state == 0) {
	    if (/$doc_start/o) {
		$state = 1;		# next line is always the function name
	    }
	} elsif ($state == 1) {	# this line is the function name (always)
	    if (/$doc_block/o) {
		$state = 4;
		$contents = "";
		if ( $1 eq "" ) {
			$section = $section_intro;
		} else {
			$section = $1;
		}
            }
	    elsif (/$doc_func/o) {
		$function = $1;
		$state = 2;
		if (/-(.*)/) {
		    $function_purpose = $1;
		} else {
		    $function_purpose = "";
		}
		if ($verbose) {
		    print STDERR "Info($lineno): Scanning doc for $function\n";
		}
	    } else {
		print STDERR "WARN($lineno): Cannot understand $_ on line $lineno",
		" - I thought it was a doc line\n";
		$state = 0;
	    }
	} elsif ($state == 2) {	# look for head: lines, and include content
	    if (/$doc_sect/o) {
		$newsection = $1;
		$newcontents = $2;

		if ($contents ne "") {
		    $contents =~ s/\&/\\\\\\amp;/g;
		    $contents =~ s/\</\\\\\\lt;/g;
		    $contents =~ s/\>/\\\\\\gt;/g;
		    dump_section($section, $contents);
		    $section = $section_default;
		}

		$contents = $newcontents;
		if ($contents ne "") {
		    $contents .= "\n";
		}
		$section = $newsection;
	    } elsif (/$doc_end/) {

		if ($contents ne "") {
		    $contents =~ s/\&/\\\\\\amp;/g;
		    $contents =~ s/\</\\\\\\lt;/g;
		    $contents =~ s/\>/\\\\\\gt;/g;
		    dump_section($section, $contents);
		    $section = $section_default;
		    $contents = "";
		}

#	    print STDERR "end of doc comment, looking for prototype\n";
		$prototype = "";
		$state = 3;
	    } elsif (/$doc_content/) {
		# miguel-style comment kludge, look for blank lines after
		# @parameter line to signify start of description
		if ($1 eq "" && $section =~ m/^@/) {
		    $contents =~ s/\&/\\\\\\amp;/g;
		    $contents =~ s/\</\\\\\\lt;/g;
		    $contents =~ s/\>/\\\\\\gt;/g;
		    dump_section($section, $contents);
		    $section = $section_default;
		    $contents = "";
		} else {
		    $contents .= $1."\n";
		}
	    } else {
		# i dont know - bad line?  ignore.
		print STDERR "WARNING($lineno): bad line: $_"; 
	    }
	} elsif ($state == 3) {	# scanning for function { (end of prototype)
	    if (m#\s*/\*\s+MACDOC\s*#io) {
	      # do nothing
	    }
	    elsif (/([^\{]*)/) {
		$prototype .= $1;
	    }
	    if (/\{/) {
		$prototype =~ s@/\*.*?\*/@@gos;	# strip comments.
		$prototype =~ s@[\r\n]+@ @gos; # strip newlines/cr's.
		$prototype =~ s@^ +@@gos; # strip leading spaces
		dump_function($prototype);

		$function = "";
		%constants = ();
		%parameters = ();
		%parametertypes = ();
		@parameterlist = ();
		%sections = ();
		@sectionlist = ();
		$prototype = "";

		$state = 0;
	    }
	} elsif ($state == 4) {
		# Documentation block
	        if (/$doc_block/) {
			dump_section($section, $contents);
			output_intro({'sectionlist' => \@sectionlist,
				      'sections' => \%sections });
			$contents = "";
			$function = "";
			%constants = ();
			%parameters = ();
			%parametertypes = ();
			@parameterlist = ();
			%sections = ();
			@sectionlist = ();
			$prototype = "";
			if ( $1 eq "" ) {
				$section = $section_intro;
			} else {
				$section = $1;
			}
                }
		elsif (/$doc_end/)
		{
			dump_section($section, $contents);
			output_intro({'sectionlist' => \@sectionlist,
				      'sections' => \%sections });
			$contents = "";
			$function = "";
			%constants = ();
			%parameters = ();
			%parametertypes = ();
			@parameterlist = ();
			%sections = ();
			@sectionlist = ();
			$prototype = "";
			$state = 0;
		}
		elsif (/$doc_content/)
		{
			if ( $1 eq "" )
			{
				$contents .= $blankline;
			}
			else
			{
				$contents .= $1 . "\n";
			}	
        	}
          }
    }
}

