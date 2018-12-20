import cv2
import numpy as np
import time
import pandas as pd 
from _datetime import datetime
import matplotlib.pyplot as plt
import matplotlib
import RPi.GPIO as GPIO
import time
import MySQLdb


GPIO.setmode(GPIO.BOARD)
GPIO.setwarnings(False)

MATRIX = [[1,2,3,'A'], #setting up 4x4 keypad
     [4,5,6,'B'],
     [7,8,9,'C'],
     ['*',0,'#','D']]

ROW = [3,5,7,11] #pins connected to rows of keypad
COL = [8,10,16,18] #pins connected to columns of keypad
GPIO.setup(37, GPIO.OUT)  #green LED output vedio is running
GPIO.setup(35, GPIO.OUT)  #LED screenshot taken

GPIO.setup(40, GPIO.IN) #from pic

face_cascade = cv2.CascadeClassifier('haarcascade_frontalface_default.xml')
smile_cascade = cv2.CascadeClassifier('haarcascade_smile.xml') 

times=[]
smile_ratios=[]
num_smiles=[]
num_faces=[]

max_ratioImg_ratio = 0
max_ratioImg_time = 0
max_ratioImg_numb = 0

max_smilesImg_ratio = 0
max_smilesImg_numb = 0
max_smilesImg_time = 0

smiletime=0
isSmile = False
isScreenshot = False
ssRatio=None
ssNumb=None
ssTime=None


for j in range(4):
    GPIO.setup(COL[j], GPIO.OUT)
    GPIO.output(COL[j], 1)
    
for i in range(4):
    GPIO.setup(ROW[i], GPIO.IN, pull_up_down = GPIO.PUD_UP)


print('access not granted')
      
access = False
while(access == False):  #wait for fingerprint
    if GPIO.input(40) == 1:
        access = True
    
print('access granted')
    
print('enter A to start')

db = MySQLdb.connect("192.168.43.57","root","root","mydb2" )


beg = False
while(beg == False): #wait for user to enter A
    
    for j in range(4):
        GPIO.output(COL[j],0)
        
        for i in range(4):
            if GPIO.input(ROW[i]) == 0:
                print(MATRIX[i][j])
                if MATRIX[i][j] == 'A':
                    beg = True
                    break
                time.sleep(0.5)
                while(GPIO.input(ROW[i])==0):
                    pass
        GPIO.output(COL[j],1)
        if beg == True:
            break

GPIO.output(37,1)  #LED turns on ... video started
start = time.time()
cap = cv2.VideoCapture(0) #connectint to camera
cap.set(cv2.CAP_PROP_FPS, 10) #setting frame per second to 10
font=cv2.FONT_HERSHEY_COMPLEX_SMALL

while True:
    try:
        ret, img = cap.read() #getting image from camera
        img = cv2.flip(img, 0) #flipping the image
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY) #adding a gray scale filter
        faces, faceRejectLevels, faceLevelWeights= face_cascade.detectMultiScale3(gray, 1.3, 5, outputRejectLevels=True) #detecting faces
        f=0
        
        sum_ratio=0
        total_smiles=0
        
        smiletime=time.time()-start
        for(x,y,w,h) in faces: 
            if (round(faceLevelWeights[f][0],3)) <= 5: continue
            cv2.rectangle(img, (x,y), (x+w, y+h), (255,0,0), 2)
            cv2.putText(img, str(round(faceLevelWeights[f][0],3)), (x,y),font, 1, (255,255,255), 2)
            roi_gray = gray[y:y+h, x:x+w]
            roi_color = img[y:y+h, x:x+w]
            
            
            smls, rejectLevels, levelWeights = smile_cascade.detectMultiScale3(roi_gray,scaleFactor=1.2,
                                                      minNeighbors=22,
                                                      minSize=(25, 25), outputRejectLevels=True) #detecting smiles for every face detected
            
            
            i=0
            
            for(ex,ey,ew,eh) in smls: #for every smile
                if(round(levelWeights[i][0],3)>=2.5): #except as a smile if the levelWeights of the first classifier are more than 2.5
                    sum_ratio += float(levelWeights[i][0])
                    print(round(levelWeights[i][0],3))       #To Display Smile Confidence
                    cv2.rectangle(roi_color, (ex,ey), (ex+ew,ey+eh), (0,255,0), 2)
                    cv2.putText(roi_color,str(round(levelWeights[i][0],3)),(ex,ey), font,1,(255,255,255),2)
                    total_smiles += 1
                    i+=1
            f+=1
            
        cv2.putText(img," num: "+str(total_smiles),(25,25), font,1,(255,255,255),2)
        for j in range(4): #wait for button B to be pressed to take a screenshot
            GPIO.output(COL[j],0)
            
            for i in range(4):
                if GPIO.input(ROW[i]) == 0:
                    print(MATRIX[i][j])
                    if MATRIX[i][j] == 'B':
                        cv2.imwrite('screenshot.jpeg',img)
                        isScreenshot = True
                        GPIO.output(35,1)
                        time.sleep(0.1)
                        GPIO.output(35,0)
                        if total_smiles != 0:
                            ssRatio=round(float(sum_ratio/total_smiles),3)
                            ssNumb=total_smiles
                        ssTime = round(smiletime, 3)
                        break;
                    while(GPIO.input(ROW[i])==0):
                        pass
            GPIO.output(COL[j],1)
            
        if max_ratioImg_ratio<sum_ratio: #storing the image with the highest average smile_ratio
            isSmile = True;
            max_ratioImg_ratio = round(sum_ratio/total_smiles, 3)
            max_ratioImg_time= round(smiletime, 3)
            max_ratioImg_numb = total_smiles
            cv2.imwrite('maxratio.jpeg',img)
            
        if max_smilesImg_numb<total_smiles: #storing the image with the highest average number of smiles
            max_smilesImg_numb = round(total_smiles, 3)
            max_smilesImg_ratio = round(sum_ratio/total_smiles, 3)
            max_smilesImg_time = round(smiletime, 3)
            cv2.imwrite('maxsmiles.jpeg',img)
            
        if len(faces) != 0: #if faces are detected
            if total_smiles != 0: #if smiles are detected
                num_faces.append(str(len(faces))) #storing the number of faces
                smile_ratios.append(round(float(sum_ratio/total_smiles),3)) #storing the average smile ratio
                num_smiles.append(total_smiles) #storing the number of smiles
                times.append(round(smiletime,3)) #storing the times at which the image is taken
            else: #if smiles are not detected
                num_faces.append(str(len(faces))) #number of faces
                num_smiles.append(0) #no smiles
                times.append(round(smiletime,3)) #time at which the image is taken
                smile_ratios.append(0.0) #average smile ratio is 0
            
        cv2.imshow('img',img) 
        
        end = time.time()
        if (end - start) > 30.0:
            print(end-start)
            break
        
        time.sleep(0.1)
        k = cv2.waitKey(30) & 0xff
        if k==27:
            break
    
    except KeyboardInterrupt:
        break

print('end of code')
GPIO.output(37,0)
ds={'smile_ratio':smile_ratios, 'num_smiles':num_smiles, 'num_faces':num_faces, 'times':times}
df=pd.DataFrame(ds)
df.to_csv('smile_recordsn.csv') #storing statistics

print(df)
print(df.dtypes)

#plotting statistics
Location = r'smile_recordsn.csv'
df = pd.read_csv(Location)

fig,ax=plt.subplots()
ax2=ax.twinx()
ax.set_ylabel('Ratio')
ax2.set_ylabel('num_smiles')

df.plot(y='smile_ratio',x='times', ax=ax)
df.plot(y='num_smiles',x='times', secondary_y=True, ax= ax2, style='g')

MaxValue = df['smile_ratio'].max()
MaxName = df['times'][df['smile_ratio'] == df['smile_ratio'].max()].apply(str).values
   
Text = str(MaxValue)+" "+MaxName

plt.annotate(Text, xy=(1, MaxValue), xytext=(8, 0), 
xycoords=('axes fraction', 'data'), textcoords='offset points')


#reading statistics to store them in database.
file = open('smile_recordsn.csv', 'r')
stats = ''
for line in file:
    stats += line
    

now = datetime.now()

formatted_date = now.strftime('%Y-%m-%d %H:%M:%S')

screenshot = None
imgdata = None
imgSmileData = None

if isScreenshot == True:
    screenshot = open('screenshot.jpeg', 'rb').read()

if isSmile == True:
    imgdata = open('maxratio.jpeg', 'rb').read()
    imgSmileData = open('maxsmiles.jpeg', 'rb').read()
    
avgRatio = round(sum(smile_ratios)/len(smile_ratios),  3)
avgNumb = round(sum(num_smiles)/len(num_smiles), 3)


cursor = db.cursor()
sql = "INSERT INTO statistics(courseID, statistics, sessionDate, maxImg, maxSmileImg, screenshot, maxImgRatio, maxImgTime, maxImgNumb, maxSmileImgRatio, maxSmileImgTime, maxSmileImgNumb,screenshotRatio, screenshotTime, screenshotNumb, avgRatio, avgNumb) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"
args = ('1', stats, formatted_date, imgdata, imgSmileData, screenshot, max_ratioImg_ratio, max_ratioImg_time, max_ratioImg_numb, max_smilesImg_ratio, max_smilesImg_time, max_smilesImg_numb, ssRatio, ssTime, ssNumb, avgRatio, avgNumb)

try:
   # Execute the SQL command
   cursor.execute(sql, args)
   # Commit your changes in the database
   db.commit()
   print("inserted")
except Exception as e:
   # Rollback in case there is any error
   print(format(e))
   db.rollback()

# disconnect from server
db.close()

plt.show()
GPIO.cleanup()
cap.release()
cv2.destroyAllWindows()
        



