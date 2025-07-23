# import uuid

# def generate_code_unique():
#     return str(uuid.uuid4())[:4]

import random
from sqlalchemy.orm import Session
from app.models.models import Employe  # Assure-toi que l'import est correct

def generate_code_unique(db: Session):
    existing_codes = {e.code_unique for e in db.query(Employe).all()}
    while True:
        code = random.randint(1000, 9999)
        if code not in existing_codes:
            return code

def generate_employee_code(db: Session):
    """Génère un code employé unique à 4 chiffres"""
    existing_codes = {e.employee_code for e in db.query(Employe).filter(Employe.employee_code.isnot(None)).all()}
    while True:
        code = str(random.randint(1000, 9999))
        if code not in existing_codes:
            return code