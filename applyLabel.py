data_file_name = 'd:/OneDrive/HCMUS/Semester_7/Physics/Project/data.csv'
label_file_name = 'd:/OneDrive/HCMUS/Semester_7/Physics/Project/label.csv'

def isPolluted(humid, temp, ppm):
    if (humid > 80 or temp > 35 or temp < 20 or ppm > 3500):
        return True
    return False

with open(data_file_name, 'r') as fr:
    with open(label_file_name, 'w') as fw:
        fr.readline()
        lines = fr.readlines()
        fw.write('temperature,humidity,ppm,label\n')
        
        for line in lines:
            measurements = line.replace('\n', '').split(',')
            humidity = float(measurements[1])
            temperature = float(measurements[0])
            ppm = float(measurements[2])
            
            label = '1' if isPolluted(humidity, temperature, ppm) else '0'
            fw.write(f'{temperature},{humidity},{ppm},{label}\n')
            
        
    