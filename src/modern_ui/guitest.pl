#!/usr/bin/perl
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# This is the Ultra Defragmenter Modern User Interface.
# Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

use strict;
use Win32::API;
use Win32::API::Callback;
use Tk;
use Tk::HList;
use Tk::ItemStyle;
use Tk::Photo;

##############  global variables ###############
my $x_blocks = 52;
my $y_blocks = 14;
my $block_size = 10;
my %colors = (chr(1) => 'white',chr(2) => 'green4', chr(3) => 'green4', chr(4) => 'red2',
	chr(5) => 'red4', chr(6) => 'blue3', chr(7) => 'blue4', chr(8) => 'magenta4',
	chr(9) => 'yellow1', chr(10) => 'yellow2', chr(11) => 'yellow3', chr(12) => 'yellow4',
	chr(13) => 'cyan1');
my $i;
my $j;
my $skip_rem = 1;
my %opts = ('x'=>200, 'y'=>200);

############# set signal handlers ###############
$SIG{INT} = sub {gui_unload(); if(udefrag_unload() < 0){ udefrag_pop_error(0,0); }};

############# load the program settings ###########
if(open(CFGFILE,'.\\my_guitest.cfg')){
	my @fields;
	foreach (<CFGFILE>){
		chomp($_);
		@fields = split(/=/);
		if(@fields == 2 && $opts{$fields[0]} ne undef){
			$opts{$fields[0]} = $fields[1];
			print "$fields[0]=$fields[1]\n";
		}
	}
	close CFGFILE;
}

############# create the main window ############
my $mw = MainWindow->new();
$mw->wm('geometry', '527x350+'.$opts{'x'}.'+'.$opts{'y'});
my $appicon = &img(0);
my $icon = $mw->Photo('image',-data=>$appicon,format=>'gif');
$mw->title('UltraDefrag v1.2.2 modern user interface');
$mw->iconimage($icon);
#display_error("привет!"); # incorrect appearance
$mw->bind(
	ref($mw),'<Destroy>',
	sub {gui_unload(); if(udefrag_unload() < 0){ udefrag_pop_error(0,0); }}
	);

############ import necessary functions from the udefrag.dll ##############
Win32::API->Import('udefrag','udefrag_init','IPIN','I') or \
	display_critical_error('Can\'t import functions from udefrag.dll!');
Win32::API->Import('udefrag','int udefrag_unload()');
Win32::API->Import('udefrag','udefrag_analyse','CK','I');
Win32::API->Import('udefrag','udefrag_defragment','CK','I');
Win32::API->Import('udefrag','udefrag_optimize','CK','I');
Win32::API->Import('udefrag','char* udefrag_get_command_result()');
Win32::API->Import('udefrag','int udefrag_stop()');
Win32::API->Import('udefrag','char* udefrag_s_get_avail_volumes(int skip_removable)');
Win32::API->Import('udefrag','char* udefrag_s_get_map(int size)');
Win32::API->Import('udefrag','udefrag_pop_error','PI');

############ fill the main window with controls ###############
#my $list = $mw->Scrolled(
#	qw/HList -header 10 -columns 6 
#	-width 58 -height 7 -scrollbars e/
#	);#->pack;

my $list = $mw->HList(
	-header => "10", -columns => "6",
	-width => "58", -height => "8",
	-background => 'black',
	-selectforeground => 'white',
	-selectbackground => 'blue',
	-selectborderwidth => '1'
	);
my @h_items = ('Volume', 'Status', 'File system', 
	'Total space', 'Free space', 'Percentage');
my @h_width = (55, 65, 100, 100, 100, 99);
for($i = 0; $i < 6; $i++){
	$list->header('create', $i, -text => $h_items[$i]);#, -relief => 'flat');
	#$list->header('configure',$i,-text => 'right');
	$list->columnWidth($i, $h_width[$i]);
}

my $label1 = $mw->Label(
	-text => 'Cluster map:'
	);

my $skip_rem_btn = $mw->Checkbutton(
	-text => 'Skip removable media',
	-variable => \$skip_rem
	);

my $rescan_btn = $mw->Button(
	-text => 'Rescan drives',
	-command => sub { rescan_drives(); }
	);

my $map = $mw->Canvas(
	-relief => 'sunken', -borderwidth => '2', -background => 'black',
	-width => '519', -height => '138'
	);
# create cells
for($j = 0; $j < $y_blocks; $j++){
	for($i = 0; $i < $x_blocks; $i++){
		$map->create(
			'rectangle', $i * $block_size + 3, $j * $block_size + 3,
			($i + 1) * $block_size + 3, ($j + 1) * $block_size + 3,
			-outline => 'black', -fill => 'white'
		);
	}
}

my $analyse_btn = $mw->Button(
	-text => 'Analyse',
	-command => sub { analyse(); }
	);

my $defrag_btn = $mw->Button(
	-text => 'Defragment',
	-command => sub { exit }
	);

my $optimize_btn = $mw->Button(
	-text => 'Optimize',
	-command => sub { exit }
	);

my $stop_btn = $mw->Button(
	-text => 'Stop',
	-command => sub { stop(); }
	);

my $fragm_btn = $mw->Button(
	-text => 'Fragmented',
	-command => sub { showreport(); }
	);

my $settings_btn = $mw->Button(
	-text => 'Settings',
	-command => sub { exit }
	);

my $about_btn = $mw->Button(
	-text => 'About',
	-command => sub { aboutbox(); }
	);

# set controls positions
$list->form(-top => '%0', -left => '%0', -right => '%100');
$label1->form(
	-top => $list, -left => '%0',
	 -right => '%20', -bottom => $map
	);
$skip_rem_btn->form(
	-top => $list, -left => $label1, 
	-right => '%80', -bottom => $map
	);
$rescan_btn->form(
	-top => $list, -left => $skip_rem_btn, 
	-right => '%100', -bottom => $map
	);
$map->form(-left => '%0', -right => '%100', -bottom => $analyse_btn);
$analyse_btn->form(-left => '%0', -right => '%25', -bottom => $fragm_btn);
$defrag_btn->form(-top => $map, -left => $analyse_btn, -right => '%50');
$optimize_btn->form(-top => $map, -left => $defrag_btn, -right => '%75');
$stop_btn->form(-top => $map, -left => $optimize_btn, -right => '%100');
$fragm_btn->form(-left => '%25', -right => '%50', -bottom => '%100');
$settings_btn->form(-left => $fragm_btn, -right => '%75', -bottom => '%100');
$about_btn->form(-left => $settings_btn, -right => '%100', -bottom => '%100');

#$mw->resizable(0,0);
$mw->minsize($mw->width,$mw->height);
$mw->maxsize($mw->width,$mw->height);

# fill list of available volumes
rescan_drives();
# initialize ultradefrag engine
handle_error(udefrag_init(0,0,0,$x_blocks * $y_blocks));

# create callback procedure
my $update_map = sub {
print $_[0]."\n";
	my $map_buffer = udefrag_s_get_map($x_blocks * $y_blocks + 1);
	$_ = $map_buffer;
	if(m/ERROR/o){
	}else{
		my $i = 0;
		my @m = split(/ */,$map_buffer); # ????????
		foreach ($map->find('all')){
			$map->itemconfigure($_,-fill => $colors{$m[$i]});
			$i++;
		}
	}
	$map->update();
	#DoOneEvent(); #??
	if($_[0] ne 0){
		$_ = udefrag_get_command_result();
		if(length($_) > 1){
			display_error($_);
		}
	}
	return 0;
};

my $update_map_callback = Win32::API::Callback->new($update_map,"N","N");

MainLoop;

############### useful subroutines ##################
sub gui_unload {
	my $key;
	my $geom;
	my @fields;
	print('Before unload...');
	# save program settings
	$geom = $mw->wm('geometry');
	@fields = split(/\+/,$geom);
	$opts{'x'} = $fields[1]; $opts{'y'} = $fields[2];
	if(open(CFGFILE,'> .\\my_guitest.cfg')){
		foreach $key (keys (%opts)) {
			if($opts{$key} eq undef){
				$opts{$key} = '';
			}
			print CFGFILE "$key=$opts{$key}\n";
		}
		close CFGFILE;
	}
}

sub display_error {
	$mw->messageBox(
		-icon => 'error', -type => 'OK',
		-title => 'Error!', -message => $_[0]
		);
}

sub display_critical_error {
	display_error($_[0]);
	die($_[0]);
}

sub handle_error {
	if($_[0] < 0){
		udefrag_pop_error(0,0);
		if(udefrag_unload(1) < 0){ udefrag_pop_error(0,0); }
		display_critical_error($_[0]);
	}
}

sub rescan_drives {
	my $item;
	my $item_style_l = $list->ItemStyle(
		'text', -anchor => 'w',
		-background => 'white',
		-selectforeground => 'white',
		);
	my $item_style_r = $list->ItemStyle(
		'text', -anchor => 'e',
		-background => 'white',
		-selectforeground => 'white',
		);
	my $col = 0;
	my $row;
	my $volumes;
	my @v;

	$volumes = udefrag_s_get_avail_volumes($skip_rem);
	$_ = $volumes;
	if(m/ERROR/o){
		display_error($volumes);
	}else{
		$list->delete('all');
		@v = split(':',$volumes);
		foreach $item (@v){
			if($col == 0){
				$row = $list->addchild("");
			}
			if($col >= 3){
				$list->itemCreate(
					$row, $col,-text => $item,
					-style => $item_style_r
					);
			}else{
				$list->itemCreate(
					$row, $col,-text => $item,
					-style => $item_style_l
					);
			}
			$col++;
			if($col == 1){
				$list->itemCreate(
					$row, $col,-text => " ",
					-style => $item_style_l
					);
				$col++; # skip the 'Status' cell
			}
			if($col > 5){
				$col = 0;
			}
		}
	}
}

sub analyse {
	my $row;
	my $letter;
	my @sel = $list->info('selection');
	if(!@sel){ return; }
	$row = $sel[0];
	$letter = $list->itemCget($row, 0, 'text');
	if(udefrag_analyse($letter, $update_map_callback) < 0){
		udefrag_pop_error(0,0);
	}
}

sub stop {
}

sub showreport {
	my $row;
	my $letter;
	my @sel = $list->info('selection');
	if(!@sel){ return; }
	$row = $sel[0];
	$letter = $list->itemCget($row, 0, 'text');
	my @args = ($letter.":\\FRAGLIST.LUAR");
	system(@args);
}

sub aboutbox {
	my $a = MainWindow->new();
	my $x = $opts{'x'} + 128;
	my $y = $opts{'y'} + 90;
	$a->wm('geometry', '282x150+'.$x.'+'.$y);
	$a->title('About Ultra Defragmenter');
#	$a->minsize($a->width,$a->height);
#	$a->maxsize($a->width,$a->height);
	my $logo = $a->Label(
		-width => '109',
		-height => '147'
		);
	my $msg = $a->Label(
		-text => "Ultra Defragmenter version 1.4.0\n\nCopyright (C) 2007,2008\n\nUltraDefrag Development Team\n"
		);
	my $credits = $a->Button(
		-text => 'Credits',
		-command => sub {}
		);
	my $license = $a->Button(
		-text => 'License',
		-command => sub {}
		);
	my $homepage = $a->Button(
		-text => 'http://ultradefrag.sourceforge.net',
		-command => sub {}
		);
	my $im = $a->Photo( -file => 't3.gif' );
	$logo->configure( -image => $im );
	$logo->form(-left => '%0');
	$msg->form(-left => $logo, -top => '%5');
	$credits->form(-left => $logo, -top => $msg);
	$license->form(-left => $credits, -top => $msg, -right => '%100');
	$homepage->form(-left => $logo, -top => $credits, -right => '%100', -bottom => '%95');
	MainLoop;
}

sub img {
# images should be in Base64 format
	my $app_gif = 
"R0lGODlhIAAgAPcAAAAAAIAAAACAAICAAAAAgIAAgACAgICAgMDAwP8AAAD/AP//AAAA//8A/wD/
/////wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMwAAZgAAmQAAzAAA/wAzAAAzMwAzZgAzmQAzzAAz/wBm
AABmMwBmZgBmmQBmzABm/wCZAACZMwCZZgCZmQCZzACZ/wDMAADMMwDMZgDMmQDMzADM/wD/AAD/
MwD/ZgD/mQD/zAD//zMAADMAMzMAZjMAmTMAzDMA/zMzADMzMzMzZjMzmTMzzDMz/zNmADNmMzNm
ZjNmmTNmzDNm/zOZADOZMzOZZjOZmTOZzDOZ/zPMADPMMzPMZjPMmTPMzDPM/zP/ADP/MzP/ZjP/
mTP/zDP//2YAAGYAM2YAZmYAmWYAzGYA/2YzAGYzM2YzZmYzmWYzzGYz/2ZmAGZmM2ZmZmZmmWZm
zGZm/2aZAGaZM2aZZmaZmWaZzGaZ/2bMAGbMM2bMZmbMmWbMzGbM/2b/AGb/M2b/Zmb/mWb/zGb/
/5kAAJkAM5kAZpkAmZkAzJkA/5kzAJkzM5kzZpkzmZkzzJkz/5lmAJlmM5lmZplmmZlmzJlm/5mZ
AJmZM5mZZpmZmZmZzJmZ/5nMAJnMM5nMZpnMmZnMzJnM/5n/AJn/M5n/Zpn/mZn/zJn//8wAAMwA
M8wAZswAmcwAzMwA/8wzAMwzM8wzZswzmcwzzMwz/8xmAMxmM8xmZsxmmcxmzMxm/8yZAMyZM8yZ
ZsyZmcyZzMyZ/8zMAMzMM8zMZszMmczMzMzM/8z/AMz/M8z/Zsz/mcz/zMz///8AAP8AM/8AZv8A
mf8AzP8A//8zAP8zM/8zZv8zmf8zzP8z//9mAP9mM/9mZv9mmf9mzP9m//+ZAP+ZM/+ZZv+Zmf+Z
zP+Z///MAP/MM//MZv/Mmf/MzP/M////AP//M///Zv//mf//zP///ywAAAAAIAAgAAAI2wABCBxI
sKDBgwUVKFzIsCGABBAjSpQosKFFhg8naoRY8eLFjBsndvToMKRGhChTHlzAsiVLAAhiypw5E4BF
gS5dwqTJM6ZNhwByttzZk+ZPjEGFLiBaVObRhSqjqmRAtarVq0mV5hR4tavVrFqHAvBKFmzYpWPJ
djUbVqrbgw7iyo0L0iRHOHjzCpw7t65dAHn1AuAr169JwIHh7CXswHBIxIHfSh5I8mZTo08rK2B6
eaTmzZdrZq7MuenkyY/VYhUqMLXqqmxbb0z7mkFsxwlov77tuvbt01IDAgA7";
	my %h = (0 => $app_gif );
	return($h{$_[0]});
}

__END__
