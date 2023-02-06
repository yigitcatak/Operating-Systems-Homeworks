# Operating-Systems-Homeworks
Sabanci University - Fall 2022 - CS307 Course

All homeworks are run in the environment of Oracle Virtualbox 6.1.40 with Ubuntu 22.04

- Homework 1: Simulate a piped shell command.
- Homework 2: Simulate shell commands. It takes input command.txt and executes each command one by one. The shell program takes care of input and output redirecting. Commands are executed concurrently depending on whether they are background processes or not. The outputs of the concurent programs are handled by a printer thread so that they are not intertwined.
- Homework 3: Simulate a rideshare application flow. Randomly generated threads replicate fans of two teams. Fans put on a rideshare request on the client thread and form a ride based on some constraits.
- Homework 4: Text corrector. Reads the database at the root folder and corrects the txt files in the all subdirectories of the root directory basde on the information given in the database.