#!/usr/bin/perl -w

# This is the Ultra Defragmenter Modern User Interface.
# Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).

use Win32::API;
use Win32::API::Callback;
use Tk;
use Tk::HList;
use Tk::ItemStyle;
use strict;

# global variables
my $x_blocks = 52;
my $y_blocks = 14;
my $block_size = 10;
my %colors = (chr(1) => 'white',chr(2) => 'green4', chr(3) => 'red2', chr(4) => 'blue3',
	chr(5) => 'magenta4', chr(6) => 'yellow1', chr(7) => 'yellow3', chr(8) => 'green4',
	chr(9) => 'red4', chr(10) => 'blue4', chr(11) => 'yellow2', chr(12) => 'yellow4',
	chr(13) => 'cyan1');
my $i;
my $j;
my $skip_rem = 1;

$SIG{INT} = sub {gui_unload(); udefrag_s_unload();};

# create the main window
my $top = MainWindow->new(
	-title => 'UltraDefrag v1.2.2 modern user interface'
	);

$top->bind(
	ref($top),'<Destroy>',
	sub {gui_unload(); udefrag_s_unload();}
	);

# import necessary functions from the udefrag.dll
my $import_result = Win32::API->Import('udefrag','char* udefrag_s_init()');
if(!$import_result){
	display_critical_error('Can\'t import functions from udefrag.dll!');
}
Win32::API->Import('udefrag','char* udefrag_s_unload()');
Win32::API->Import('udefrag','udefrag_s_analyse_ex','CK','P');
Win32::API->Import('udefrag','udefrag_s_defragment_ex','CK','P');
Win32::API->Import('udefrag','udefrag_s_optimize_ex','CK','P');
Win32::API->Import('udefrag','char* udefrag_s_stop()');
Win32::API->Import('udefrag','char* udefrag_s_get_avail_volumes(int skip_removable)');
Win32::API->Import('udefrag','char* udefrag_s_get_progress()');
Win32::API->Import('udefrag','char* udefrag_s_get_map(int size)');
Win32::API->Import('udefrag','char* udefrag_s_load_settings()');
Win32::API->Import('udefrag','char* udefrag_s_get_settings()');
Win32::API->Import('udefrag','char* udefrag_s_apply_settings(char* string)');
Win32::API->Import('udefrag','char* udefrag_s_save_settings()');

# fill the main window with controls
#my $list = $top->Scrolled(
#	qw/HList -header 10 -columns 6 
#	-width 58 -height 7 -scrollbars e/
#	);#->pack;

my $list = $top->HList(
	-header => "10", -columns => "6",
	-width => "58", -height => "8",
	-background => 'black',
	-selectforeground => 'white',
	-selectbackground => 'blue',
	-selectborderwidth => '1'
	);
my @h_items = ('Volume ', 'Status ', 'File system', 
	'   Total space', '    Free space', 'Percentage');
my @h_width = (55, 65, 100, 100, 100, 99);
for($i = 0; $i < 6; $i++){
	$list->header('create', $i, -text => $h_items[$i]);#, -relief => 'flat');
	$list->columnWidth($i, $h_width[$i]);
}

my $label1 = $top->Label(
	-text => 'Cluster map:'
	);

my $skip_rem_btn = $top->Checkbutton(
	-text => 'Skip removable media',
	-variable => \$skip_rem
	);

my $rescan_btn = $top->Button(
	-text => 'Rescan drives',
	-command => sub { rescan_drives(); }
	);

my $map = $top->Canvas(
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

my $analyse_btn = $top->Button(
	-text => 'Analyse',
	-command => sub { analyse(); }
	);

my $defrag_btn = $top->Button(
	-text => 'Defragment',
	-command => sub { exit }
	);

my $optimize_btn = $top->Button(
	-text => 'Optimize',
	-command => sub { exit }
	);

my $stop_btn = $top->Button(
	-text => 'Stop',
	-command => sub { exit }
	);

my $fragm_btn = $top->Button(
	-text => 'Fragmented',
	-command => sub { exit }
	);

my $settings_btn = $top->Button(
	-text => 'Settings',
	-command => sub { exit }
	);

my $about_btn = $top->Button(
	-text => 'About',
	-command => sub { exit }
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

# fill list of available volumes
rescan_drives();
# initialize ultradefrag engine
handle_error(udefrag_s_init());

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
	return 0;
};

my $update_map_callback = Win32::API::Callback->new($update_map,"N","N");

MainLoop;

# useful subroutines
sub gui_unload {
	print('Before unload...');
}

sub display_error {
	$top->messageBox(
		-icon => 'error', -type => 'OK',
		-title => 'Error!', -message => $_[0]
		);
}

sub display_critical_error {
	display_error($_[0]);
	die($_[0]);
}

sub handle_error {
	if(length($_[0])){
		udefrag_s_unload();
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
	udefrag_s_analyse_ex($letter, $update_map_callback);
}

__END__
