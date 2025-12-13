from sklearn.model_selection import train_test_split
from sklearn.tree import DecisionTreeClassifier

import pickle
import pandas as pd
df = pd.read_csv('dataset.csv')

X = df.iloc[:, :-1]
y = df.iloc[:, -1]

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)
model = DecisionTreeClassifier(criterion="entropy", max_depth=5, random_state=42)
model.fit(X_train, y_train)
pickle.dump(model, open('model.pkl', mode='wb'))