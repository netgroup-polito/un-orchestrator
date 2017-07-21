#!/usr/bin/env php
<?php
/*
	How to launch:
		Dependencies:	apt-get install php-cli

		A)	chmod +x ovs_dump_flows.php
			./ovs_dump_flows.php BridgeName

			OR

		B)	php ./ovs_dump_flows.php

*/

if ( count($argv) <= 1 )
{
	echo "USAGE: $argv[0] [BridgeName]\n";
	die();
}

$bridge = $argv[1];

$map_ports=array();

exec("ovs-ofctl -O OpenFlow13 dump-ports-desc $bridge",$output);
foreach($output as $line)
{
	if (strpos($line,"(") !== FALSE)
	{
		$exp = explode(":",$line);
		$p = str_replace(")","",trim($exp[0]));
		$exp = explode("(",$p);
		$number = $exp[0];
		$port = $exp[1];
		$map_ports[$number] = $port;
	}
}

$output=array();
exec("ovs-ofctl dump-flows -O OpenFlow13 $bridge | grep -v OFPST_FLOW ",$output);
foreach($output as $line)
{
	$line = str_replace(" actions",", actions",$line);

	$attributes = explode(",",$line);

	foreach ( $attributes as $attribute )
	{
		$attribute=trim($attribute);
		$v = explode("=",$attribute);
		$name = $v[0];
		$value = "";
		if ( isset($v[1]) )
		{
			$value = $v[1];
		}

		if ( $name == "in_port" )
		{
			if ( isset($map_ports[$value]) )
			{
				$value = $map_ports[$value];
			}
			else
			{
				echo "\nERR: Unable to find name for port '$value'\n";
			}
		}
		else if ( $name == "actions" )
		{
			//TODO better
			$action_attributes = explode(",",$value);

			$value="";
			foreach ( $action_attributes as $action_attribute )
			{
				$e = explode(":",$action_attribute);
				$action_attribute_name=$e[0];
				$action_attribute_value=$e[1];

				if ( $action_attribute_name == "output" )
				{
					if ( isset($map_ports[$action_attribute_value]) )
					{
						$action_attribute_value = $map_ports[$action_attribute_value];
					}
					else
					{
						echo "\nERR: Unable to find name for port '$action_attribute_value'\n";
					}
					$value .= "$action_attribute_name:".$action_attribute_value.",";
				}
				else
				{
					echo "\nERR: Unknown action attribute with name '$name'\n";
				}
			}
			$value = rtrim($value,","); //Remove the last comma
		}
		else if ( $name == "cookie" )
		{
		}
		else if ( $name == "duration" )
		{
		}
		else if ( $name == "table" )
		{
		}
		else if ( $name == "n_packets" )
		{
		}
		else if ( $name == "n_bytes" )
		{
		}
		else if ( $name == "priority" )
		{
		}
		else if ( $name == "ip" )
		{
		}
		else if ( $name == "dl_dst" )
		{
		}
		else if ( $name == "dl_src" )
		{
		}
		else if ( $name == "nw_src" )
		{
		}
		else if ( $name == "nw_dst" )
		{
		}
		else
		{
			echo "\nERR: Unknown attribute with name '$name'\n";
		}

		echo $name."=".$value." ";
	}
	echo "\n";
}
?>
