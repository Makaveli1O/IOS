#!/bin/bash
POSIXLY_CORRECT=yes
args=$#			#$0 -> drigraph.sh $1 -> [-i FILE_ERE] $2 -> [-n] $3 -> [DIR]

#--------TO DO ----------------#
# UPRAVIT VYPIS PODLA ZADANIA !!!!
# -n normalizaciu pre 15tisic suborov spravit dajako :D  
# dlzka terminalu je este dlzku ked terminal nieje

#-----------------------------------------------------------------------------#
#-------------------------------ARG - HANDLING -------------------------------#
#-----------------------------------------------------------------------------#
i_filter=False
n_filter=False
i_val=""
if [[ $args -eq 0 ]]
then
	filters=False
else
	filters=True
	while getopts ":i:n" arg
       	do	case "$arg" in
		i) i_filter=True i_val=$OPTARG;;
		n) n_filter=True;;
		*) echo 'ERROR: Argument missing after: "-i"'
		   exit 1;;
		esac
	done
fi
shift $((OPTIND-1))
DIR=$1
ND=0
NF=0
CURRENT_DIR=False


#if there is no given root directory, use current directory as root
if [[ -z $DIR ]]	
then
	CURRENT_DIR=True
	DIR=$PWD
fi

#if -i parameter and root directory are same, Error.
if [[ "$i_val" == "$DIR" ]]
then
	echo ERROR: Parameter given to -i is same as root diretory.
	exit 1
fi

#given directory does not exist
if  [[ ! -d $DIR ]]
then
	echo ERROR: Given directory does not exist.
	exit 1
fi

#-n after -i
if [[ $i_val == "-n" ]]
then
	echo 'ERROR: Argument missing after: "-i"'
	exit 1
fi

#check whenever user is using Terminal or not.
if [ -t 0 ]
then
	TERMINAL=True
	line_len=$((`tput cols` - 1))
else
	TERMINAL=False
	#nieco_len=nieco
fi

#-----------------------------------------------------------------------------#
#-----------------------------FUNCTIONS DEFINITIONS---------------------------#
#-----------------------------------------------------------------------------#
#Upraviť výpis PODLA ZADANIE!!!!
function Output(){

	echo "Root Directory: "$DIR
	echo "Directories: "$ND 	#number with current directory
	echo "All Files: "$NF		#number with hidden files(.DS_store on mac)
	echo "File size histogram: "
	echo "	<100 B	: "$LT_100B
	echo "	<1 KiB	: "$LT_1KiB
	echo "	<10 KiB	: "$LT_10KiB
	echo "	<1 MiB	: "$LT_1MiB
	echo "	<10 MiB	: "$LT_10MiB
	echo "	<100 MiB: "$LT_100MiB
	echo "	<1 GiB	: "$LT_1GiB
	echo "	>=1 GiB	: "$GE_1GiB

	return

}

function Rounding(){
	# $1 -> hashtags
	# $2 -> num of hashtags
	num_hashes=$2	
	if $TERMINAL
	then
		if [[ $num_hashes -eq 0 ]]
		then
			num_hashes=0
		else
			residue=$((num_hashes%10))
			num_hashes=$((num_hashes/10))
			[[ $num_hashes -eq 0 ]] && num_hashes=1 || num_hashes="$num_hashes"
		fi
		#if normalization is still not enough recursively call Rounding again
		#[[ $num_hashes -gt $line_len ]] && Rounding $1 $num_hashes 
		[[ $num_hashes -gt $line_len ]] && printf $num_hashes
		if [[ $num_hashes -eq 0 ]]
		then
			echo ""
		else
			[[ $residue -gt 5 ]] && let "num_hashes++" || num_hashes="$num_hashes"
		
		echo $1 | cut -c1-$num_hashes
		fi
	#TERMINAL is not being used
	else
		if [[ $num_hashes -eq 0 ]]
		then
			num_hashes=0
		else
			residue=$((num_hashes%10))
			num_hashes=$((num_hashes/10))
			[[ $num_hashes -eq 0 ]] && num_hashes=1 || num_hashes="$num_hashes"
		fi
		
		#if normalization is still not enough recursively call Rounding again
		[[ $num_hashes -gt 79 ]] && printf $num_hashes

		if [[ $num_hashes -eq 0 ]]
		then
			echo ""
		else
			[[ $residue -gt 5 ]] && let "num_hashes++" || num_hashes="$num_hashes"
		
		echo $1 | cut -c1-$num_hashes
		fi
	fi

}

function Round(){
	let "count++"
	overflow=False
	LT_100B=$( Rounding $LT_100B $B100 )
	
	LT_1KiB=$( Rounding $LT_1KiB $KiB1 )
	
	LT_10KiB=$( Rounding $LT_10KiB $KiB10 )
	
	LT_100KiB=$( Rounding $LT_100KiB $KiB100 )
	
	LT_1MiB=$( Rounding $LT_1MiB $MiB1 )
	
	LT_10MiB=$( Rounding $LT_10MiB $MiB10 )
	
	LT_100MiB=$( Rounding $LT_100MiB $MiB100 )
	
	LT_1GiB=$( Rounding $LT_1GiB $GiB1_L )
	
	GE_1GiB=$( Rounding $GE_1GiB $GiB1_G )
	
	#overflow
	if [[ $LT_100B =~ ^[0-9] ]]
	then
		B100=$(echo $LT_100B | cut -f1 -d"#")
		LT_100B=$(echo $LT_100B | cut -d "#" -f2)
		overflow=True
	elif [[ $LT_1KiB =~ ^[0-9] ]]
	then
		KiB1=$(echo $LT_1KiB | cut -f1 -d"#")
		LT_1KiB=$(echo $LT_1KiB | tr -d '0123456789') #trim number
		overflow=True
	elif [[ $LT_10KiB =~ ^[0-9] ]]
	then
		KiB10=$(echo $LT_10KiB | cut -f1 -d"#")
		LT_10KiB=$(echo $LT_10KiB | tr -d '0123456789') #trim number
		overflow=True
	elif [[ $LT_100KiB =~ ^[0-9] ]]
	then
		KiB100=$(echo $LT_100KiB | cut -f1 -d"#")
		LT_100KiB=$(echo $LT_100KiB | tr -d '0123456789') #trim number
		overflow=True
	elif [[ $LT_1MiB =~ ^[0-9] ]]
	then
		MiB1=$(echo $LT_1MiB | cut -f1 -d"#")
		LT_1MiB=$(echo $LT_1MiB | tr -d '0123456789') #trim number
		overflow=True
	elif [[ $LT_10MiB =~ ^[0-9] ]]
	then
		MiB10=$(echo $LT_10MiB | cut -f1 -d"#")
		LT_10MiB=$(echo $LT_10MiB | tr -d '0123456789') #trim number
		overflow=True
	elif [[ $LT_100MiB =~ ^[0-9] ]]
	then
		MiB100=$(echo $LT_100MiB | cut -f1 -d"#")
		LT_100MiB=$(echo $LT_100MiB | tr -d '0123456789') #trim number
		overflow=True
	elif [[ $LT_1GiB =~ ^[0-9] ]]
	then 
		GiB1_L=$(echo $LT_1GiB | cut -f1 -d"#")
		LT_1GiB=$(echo $LT_1GiB | tr -d '0123456789') #trim number
		overflow=True
	elif [[ $GE_1GiB =~ ^[0-9] ]]
	then 
		GiB1_G=$(echo $GE_1GiiB | cut -f1 -d"#")
		GE_1GiB=$(echo $GE_1GiB | tr -d '0123456789') #trim number
		overflow=True
	
	fi

	#recursive call if overflow happened after -n
	if $overflow
	then
		Round
	fi
		
}
#-----------------------------------------------------------------------------#
#-------------------------------START OF MAIN---------------------------------#
#-----------------------------------------------------------------------------#

ND=$(find $DIR -type d | wc -l) 		#this includes current directory !!!
NF=$(find $DIR -type f | wc -l) 		#this includes hidden files as well(starting with .) !!!

#	-	-	-	-	-	#
#Sizes sorting and arrangement recursively	#
#	-	-	-	-	-	#

if $CURRENT_DIR
then
	if $i_filter
	then
		i_val=$(echo $i_val | sed 's;/;\\/;g')
		file_size=$(find . -type f -ls | sed '/\.\/'$i_val'/, /^$/'d |  awk '{print $7}')
	else
		file_size=$(find . -type f -ls | egrep -v '^d' | awk '{print $7}')
	fi
else
	if $i_filter
	then
		DIR2=$(echo $DIR | sed 's;/;\\/;g')
		i_val=$(echo $DIR | sed 's;/;\\/;g')	
		file_size=$(find $DIR -type f -ls | sed "/$DIR2\/$i_val/,/^$/"d | egrep -v '^d' | awk '{print $7}')
	else
		file_size=$(find $DIR -type f -ls | egrep -v '^d' | awk '{print $7}')
	fi
fi

file_sizes=( $file_size )

LT_100B=()
LT_1KiB=()
LT_10KiB=()
LT_100KiB=()
LT_1MiB=()
LT_10MiB=()
LT_100MiB=()
LT_1GiB=()
GE_1GiB=()

B100=0
KiB1=0
KiB10=0
KiB100=0
MiB1=0
MiB10=0
MiB100=0
GiB1_L=0
GiB1_G=0

arraylength=${#file_sizes[@]}
for (( i=0; i<${arraylength}; i++ ))
do	
	value=${file_sizes[i]}
	
	if [[ $value -lt 100 ]]
	then
		B100=$((B100+=1))
		LT_100B+="#"
		continue
	elif [ $value -ge 100 -a $value -lt 1024 ]
	then
		KiB1=$((KiB1+=1))
		LT_1KiB+="#"
		continue
	elif [ $value -ge 1024 -a $value -lt 10240 ]
	then
		KiB10=$((KiB10+=1))
		LT_10KiB+="#"
		continue
	elif [ $value -ge 10240 -a $value -lt 102400 ]
	then
		KiB100=$((KiB100+=1))
		LT_100KiB+="#"
		continue
	elif [ $value -ge 102400 -a $value -lt 1048576 ]
	then
		MiB1=$((MiB1+=1))
		LT_1MiB+="#"
		continue
	elif [ $value -ge 1048576 -a $value -lt 10485760 ]
	then
		MiB10=$((MiB10+=1))
		LT_10MiB+="#"
		continue
	elif [ $value -ge 10485760 -a $value -lt 104857600 ]
	then
		MiB100=$((MiB100+=1))
		LT_100MiB+="#"
		continue
	elif [ $value -ge 104857600 -a $value -lt 1048576000 ]
	then
		GiB1_L=$((GiB1_L+=1))
		LT_1GiB+="#"
		continue
	elif [ $value -ge 1048576000 ]
	then
		GiB1_G=$((GiB1_G+=1))
		GE_1GiB+="#"
		continue
	fi
	
done
#-------------------OUTPUT------------------#
#normalization of histogram "-n" is called
if $n_filter
then
	#if any line is greater than terminal length otherwise just print histogram
	if [ $B100 -gt $line_len ] || [ $KiB1 -gt $line_len ] || [ $KiB10 -gt $line_len ] || [ $KiB100 -gt $line_len ] || [ $MiB1 -gt $line_len ] || [ $MiB10 -gt $line_len ] || [ $MiB100 -gt $line_len ] || [ $GiB1_L -gt $line_len ] || [ $GiB1_G -gt $line_len ]
	then 
		count=0
		Round $count
		Output
	else
		Output
	fi
else
	Output
fi
#-----------------------------------------------------------------------------#
#-------------------------------END- OF- MAIN---------------------------------#
#-----------------------------------------------------------------------------#
