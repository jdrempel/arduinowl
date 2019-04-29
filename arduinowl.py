import json
import math
import serial
import time
import urllib.request

ser = serial.Serial()
ser.baudrate = 9600
# ser.baudrate = 2400
ser.port = '/dev/ttyUSB0'
ser.timeout = 2
ser.close()
# ser.open()
time.sleep(2)

while True:

    teamdata = urllib.request.urlopen('https://api.overwatchleague.com/teams')
    scheduledata = urllib.request.urlopen('https://api.overwatchleague.com/schedule')
    rankdata = urllib.request.urlopen('https://api.overwatchleague.com/rankings')
    teams = json.loads(teamdata.read().decode())
    schedule = json.loads(scheduledata.read().decode())
    rankings = json.loads(rankdata.read().decode())

    x = ''
    now = math.floor(time.time())*1000

    # TODO: Refine this algorithm for a better way of checking ahead for 3 matches

    for stage in schedule['data']['stages']:
        stage_out = 0
        for week in stage['weeks']:
            stage_out = max(stage_out, week['endDate'])
        if now < stage_out:
            for week in stage['weeks']:
                if now < week['endDate']:
                    # this is new
                    matches_highlighted = 0
                    # end new
                    for k, match in enumerate(week['matches']):
                        if match['startDateTS'] > now:
                            x += '<Next: '+str(match['competitors'][0]['abbreviatedName'])+' vs '\
                                + str(match['competitors'][1]['abbreviatedName']) + '*'
                            x = x + '>' + str(time.strftime('%a %b %d %H:%M', time.localtime(match['startDateTS']/1000))) + '*'
                            matches_highlighted += 1
                            # break
                        elif match['endDateTS'] > now:
                            x += '<Live: '+str(match['competitors'][0]['abbreviatedName'])+' vs '\
                                + str(match['competitors'][1]['abbreviatedName']) + '*'
                            x = x + '>' + str(match['scores'][0]['value'])+' - '+str(match['scores'][1]['value']) + '*'
                            matches_highlighted += 1
                            # break
                        # this is new
                        if matches_highlighted == 3:
                            break
                        # end new
                    break
            break
    
    for p, team in enumerate(rankings['content']):
        mdiff = team['records'][0]['gameWin'] - team['records'][0]['gameLoss']
        if mdiff > 0:
            mdiff = '+' + str(mdiff)
        y = '<' + str(team['placement'])+'. '+str(team['competitor']['abbreviatedName'])+' '\
            + str(team['records'][0]['matchWin'])\
            + '-' + str(team['records'][0]['matchLoss']) + '*'
        y = y + '>Map Diff: ' + str(mdiff) + '*'
        x = x + y
    print(x)
    x = bytes(x, 'utf-8')
    # ser.write(x)
    time.sleep(5)
    # r = ser.read_all()
    # print(str(r,'ascii'))
