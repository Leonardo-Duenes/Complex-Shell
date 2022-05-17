# Shell
### CS 4348.004  - Project 2  
Team: 
David Nguyen : dxn180015 
Dueñes Gomez : lxd180007 
Luiz Astorga : laa180001

Notes:
We completed the bonus.

Instructions:
1) Download the files into the cs1 machine into the same folder
2) Run the 'make' command   
3) Call this command to run the program './my-shell'

Test Cases:
cat < inputBob.txt | grep Bob | sort -r > outputBob.txt 
ls | less (quit with 'q')
cat *file1 file2 file3* | wc -l
ls -al | grep txt | less
ls | grep txt >> output21.txt
cat < input.txt > output22.txt
ls | grep txt | sort -r > o.txt

Test Case For Bonus:
pushd /etc
pushd /var
dirs
popd
popd
dirs