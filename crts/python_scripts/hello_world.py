#!/usr/bin/python
import random
import os
import sys


print("Hello World");
name = "Shem";
print(name);

x = 3%2

print("x is ", x)

'''
    This is a comment
'''

#This is a comment

# ** power, // floor division

y = 6//5

print('y is ', y)


multi_line_quote = '''life is wierd
but who cares'''

print("%s" % (multi_line_quote))

grocery_list = ['Juice', 'Oranges','Apples']

print('First 2 Items', grocery_list[0:2])

other_events = ['Walk Dog',5,'Change Nappy']
to_do_list = [other_events,grocery_list]
grocery_list.append('Onions')
grocery_list.remove('Oranges')
grocery_list.reverse()
print(to_do_list)


print('Length of list:',len(to_do_list))


#tuples do no Change

first_tuple = (1,2,'h',12,'u')
first_list = list(first_tuple)

new_tuple = tuple(first_list)


#Dictionaries are like hashmaps


super_villains = {'Fiddler':'Isaac Bown','Captain Cold':'Leo'}


print('Captain Cold',super_villains['Captain Cold'])


# conditions

age = 21

if age > 16 :
    print("Age is above 16")
else :
    print("Age is above 16")

if age >=21 :
    print("Drink!")

elif age >= 16 :
    print("Yeah!! Above 16")
else :
    print("Too Young!")


if ((age >=1) and (age <= 18)):
    print("Below Age")
else:
    print("Too Old")

# For loops
for x in range(0,10):
    sys.stdout.write("%s " % x)
sys.stdout.write("\n")

sys.stdout.flush()

for y in grocery_list:
    print(y)

num_list = [[1,2,3],[1,2,3],[1,2,3]]

for x in num_list:
    for y in x:
        sys.stdout.write("%s" % y)
    sys.stdout.write("\n")


# While Loops

random_num = random.randrange(0,16)

while (random_num != 15):
    print(random_num)
    random_num = random.randrange(0,16)

i = 0

while i <= 20:
    if(i%2 == 0):
        print("%s" % i )

    i = i + 1


# functions


def addNumber(num1,num2):
    sumNum = num1 + num2
    return sumNum

addSum = addNumber(3,4)

print(("Added Sum of %s + %s" % (3,4) ),addSum)


# user input

print("Give me something")
input = sys.stdin.readline()

print("Is this what you gave me: %s" %input)


#string

long_string = "Yo, you cool?"

print(long_string[-5:])

test_file = open("test.txt","wb")

test_file.write("Hahahaha\n")

test_file.close()

test_in_file = open("test.txt","r+")


print(test_in_file.read())

#os.remove("test.txt")
