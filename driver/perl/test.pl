use QConf;
use strict;
# init
my $err = &QConf::qconf_init();
if ($err != 0){
	printf "init fail.\n";
	printf "$err\n";
}


#qconf_get_conf
	my $ret = "";
	my $errCode = &QConf::get_conf("/zookeeper",$ret,"","true");
	if ($errCode == 0){  
		printf "$ret\n";	
	}else{
		printf "fail,err: %d\n",$errCode;
	}

#qconf_get_batch_conf
	my %ret = ();
	my $errCode = &QConf::get_batch_conf("/zookeeper",\%ret,"","false");
	if ($errCode == 0){  		
		while (my ($k, $v) = each %ret){
    		print "$k : $v\n";
    	}
	}else{
		printf "fail,err: %d\n",$errCode;
	}

#qconf_get_host
	my $ret = "";
	my $errCode = &QConf::get_host("/zookeeper",$ret,"","false");
	if ($errCode == 0){  
		printf "$ret\n";	
	}else{
		printf "fail,err: %d\n",$errCode;
	}

#qconf_get_batch_keys
	my @ret = ();
	my $errCode = &QConf::get_batch_keys("/zookeeper",\@ret,"","false");
	if ($errCode == 0){  		
		foreach(@ret){
			printf "$_\n";
		}
	}else{
		printf "fail,err: %d\n",$errCode;
	}

#finalize
&QConf::qconf_destroy();

