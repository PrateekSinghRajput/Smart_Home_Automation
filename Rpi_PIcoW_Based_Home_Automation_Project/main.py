from machine import UART, Pin


gsm_module = UART(1, baudrate=9600, tx=Pin(4), rx=Pin(5), timeout=2000)
gsm_buffer = ''


destination_phone = '+918830584864'


relay1 = Pin(2, Pin.OUT)
relay2 = Pin(3, Pin.OUT)

def convert_to_string(buf):
    tt =  buf.decode('utf-8').strip()
    return tt

def do_action(msg):
    msg = msg.lower()
    if(msg.strip() == 'light off'):
        relay1(0)
        send_sms('light is OFF')
    elif(msg.strip() == "light on"):
        relay1(1)
        send_sms('light is ON')
    elif(msg.strip() == 'fan off'):
        relay2(0)
        send_sms('fan is OFF')
    elif(msg.strip() == 'fan on'):
        print('do_action1: '+msg)
        relay2(1)
        send_sms('fan is ON')
def send_command(cmdstr, lines=1, msgtext=None):
    global gsm_buffer
    print(cmdstr)
    cmdstr = cmdstr+'\r\n'
    
    while gsm_module.any():
        gsm_module.read()
    
    gsm_module.write(cmdstr)
    
    if msgtext:
        print(msgtext)
        gsm_module.write(msgtext)
 
    buf=gsm_module.readline() 
    buf=gsm_module.readline()
    if not buf:
        return None
    result = convert_to_string(buf)
    
    if lines>1:
        gsm_buffer = ''
        for i in range(lines-1):
            buf=gsm_module.readline()
            if not buf:
                return result
            buf = convert_to_string(buf)
            if not buf == '' and not buf == 'OK':
                gsm_buffer += buf+'\n'
    
    return result
def read_sms(sms_id):
    result = send_command('AT+CMGR={}\n'.format(sms_id),99)
    print(result)
    
    if result:
        params=result.split(',')
        if params[0] == '':
           return None
        
        params2 = params[0].split(':')
        if not params2[0]=='+CMGR':
            return None
        
        number = params[1].replace('"',' ').strip()
        date   = params[3].replace('"',' ').strip()
        time   = params[4].replace('"',' ').strip()
        #print('gsm_buffer:'+gsm_buffer)
        return  [number,date,time,gsm_buffer]
      
def send_sms(msgtext):
    global gsm_buffer
    result = send_command('AT+CMGS="{}"\n'.format(destination_phone),99,msgtext+'\x1A')
    if result and result=='>' and gsm_buffer:
        params = gsm_buffer.split(':')
        if params[0]=='+CUSD' or params[0] == '+CMGS':
            print('OK')
            return 'OK'
    print('ERROR')
    return 'ERROR'
print(send_command('AT'))
print(send_command('AT+CMGF=1'))
print(send_command('AT+CNMI=1'))

while True:
    if gsm_module.any():
        buf=gsm_module.readline()
        buf = convert_to_string(buf)
        print(buf)
        
        params=buf.split(',')
      
        if params[0][0:5] == "+CMTI":
            msgid = int(params[1])
            msg_data = read_sms(msgid)
            
            if not msg_data:
                print("No sms data found.")
                break
          
            print(msg_data[3])
            
            if not msg_data[0] == destination_phone:
                print("Destination phone pumber not matching")
                break
           
            do_action(msg_data[3])
           
