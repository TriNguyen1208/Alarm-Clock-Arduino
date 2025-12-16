import firebase_admin
from firebase_admin import credentials, db

cred = credentials.Certificate('C:/Users/daokh/.node-red/serviceAccountKey.json')

firebase_admin.initialize_app(cred, {
    "databaseURL": "url_here"
})

ref = db.reference("/TrainingData")

data = ref.get()

file_name = 'd:/OneDrive/HCMUS/Semester_7/Physics/Project/data.csv'
count = len(data)
print(f'Number of samples: {count}')

if data is not None:
    with open(file_name, 'w') as f:
        f.write('temperature,humidity,ppm\n')
        for data_dict in data.values():
            humidity = data_dict['humidity']
            temperature = data_dict['temperature']
            ppm = data_dict['ppm']
            f.write(f'{temperature},{humidity},{ppm}\n')