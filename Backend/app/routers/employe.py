from fastapi import APIRouter, Depends, HTTPException, FastAPI
from sqlalchemy.orm import Session
from app.database import SessionLocal
from app.models.models import Employe, Presence, Empreinte
from app.schemas.schemas import (
    EmployeCreate, EmployeRead, FingerprintVerificationData,
    EmpreintesUpdate, EmpreintesResponse, PresenceCreate, PresenceRead
)
from app.utils import generate_code_unique, generate_employee_code
from typing import List
from datetime import datetime, date
import hashlib
from base64 import b64decode
import uuid

# Définition du router
router = APIRouter(prefix="/employes", tags=["Employes"])

# Dépendance à la base de données
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

@router.post("/", response_model=EmployeRead)
def create_employe(employe: EmployeCreate, db: Session = Depends(get_db)):
    code_unique = generate_code_unique(db)
    employee_code = generate_employee_code(db)
    
    # Créer les données de l'employé
    employe_data = employe.dict()
    employe_data['code_unique'] = code_unique
    employe_data['employee_code'] = employee_code
    
    new_employe = Employe(**employe_data)
    db.add(new_employe)
    db.commit()
    db.refresh(new_employe)
    return new_employe

@router.get("/", response_model=List[EmployeRead])
def get_all_employes(db: Session = Depends(get_db)):
    employes = db.query(Employe).all()
    return employes

@router.get("/verify-code/{code_unique}")
def verify_code_unique(code_unique: int, db: Session = Depends(get_db)):
    employe = db.query(Employe).filter(Employe.code_unique == code_unique).first()

    if not employe:
        return {"status": "error", "message": "Code Invalide"}

    if employe.fingerprints_enrolled < 2:
        return {
            "status": "enrollment_required",
            "message": "enrolled again",
            "employe_id": str(employe.id),
            "full_name": employe.full_name
        }

    return {
        "status": "pointage",
        "message": "rady for pointage.",
        "employe_id": str(employe.id),
        "full_name": employe.full_name
    }

# Route 1: Obtenir l'ID d'un utilisateur par employee_code
@router.get("/by-code/{employee_code}")
def get_user_id_by_employee_code(employee_code: str, db: Session = Depends(get_db)):
    employe = db.query(Employe).filter(Employe.employee_code == employee_code).first()
    
    if not employe:
        return {"user_id": None, "message": "Aucun employé trouvé avec ce code"}
    
    return {"user_id": str(employe.id), "full_name": employe.full_name}

# Route 2: Mettre à jour les empreintes d'un utilisateur
@router.put("/{user_id}/empreintes")
def update_user_empreintes(user_id: str, empreintes_data: EmpreintesUpdate, db: Session = Depends(get_db)):
    try:
        user_uuid = uuid.UUID(user_id)
    except ValueError:
        raise HTTPException(status_code=400, detail="ID utilisateur invalide")
    
    employe = db.query(Employe).filter(Employe.id == user_uuid).first()
    if not employe:
        raise HTTPException(status_code=404, detail="Utilisateur non trouvé")
    
    # Supprimer les anciennes empreintes
    db.query(Empreinte).filter(Empreinte.employe_id == user_uuid).delete()
    
    # Ajouter les nouvelles empreintes
    for empreinte_id in empreintes_data.empreinte_ids:
        new_empreinte = Empreinte(employe_id=user_uuid, empreinte_id=empreinte_id)
        db.add(new_empreinte)
    
    # Mettre à jour le statut d'enregistrement des empreintes
    employe.fingerprints_enrolled = len(empreintes_data.empreinte_ids) >= 2
    
    db.commit()
    
    return {"message": f"Empreintes mises à jour avec succès. {len(empreintes_data.empreinte_ids)} empreinte(s) enregistrée(s)"}

# Route 3: Obtenir les empreintes d'un employé par employee_code
@router.get("/{employee_code}/empreintes", response_model=EmpreintesResponse)
def get_empreintes_by_employee_code(employee_code: str, db: Session = Depends(get_db)):
    employe = db.query(Employe).filter(Employe.employee_code == employee_code).first()
    
    if not employe:
        raise HTTPException(status_code=404, detail="Employé non trouvé")
    
    empreintes = db.query(Empreinte).filter(Empreinte.employe_id == employe.id).all()
    empreinte_ids = [emp.empreinte_id for emp in empreintes]
    
    return EmpreintesResponse(empreinte_ids=empreinte_ids)

# Route 4: Signaler présence
@router.post("/presences/signal", response_model=PresenceRead)
def signal_presence(presence_data: PresenceCreate, db: Session = Depends(get_db)):
    # Vérifier que l'employé existe
    employe = db.query(Employe).filter(Employe.employee_code == presence_data.employee_code).first()
    if not employe:
        raise HTTPException(status_code=404, detail="Employé non trouvé avec ce code")
    
    # Obtenir l'heure actuelle
    now = datetime.now()
    
    # Créer la présence avec les données automatiques
    new_presence = Presence(
        employe_id=employe.id,
        employee_code=presence_data.employee_code,
        date_presence=now.date(),
        heure_arrivee=now,
        heure_depart=None,
        type_presence="entrée",
        notes=None
    )
    
    db.add(new_presence)
    db.commit()
    db.refresh(new_presence)
    
    return new_presence

# Route pour obtenir toutes les présences d'un employé
@router.get("/{employee_code}/presences", response_model=List[PresenceRead])
def get_employe_presences(employee_code: str, db: Session = Depends(get_db)):
    employe = db.query(Employe).filter(Employe.employee_code == employee_code).first()
    
    if not employe:
        raise HTTPException(status_code=404, detail="Employé non trouvé")
    
    presences = db.query(Presence).filter(Presence.employe_id == employe.id).all()
    return presences

# Route pour modifier l'employee_code d'un employé
@router.put("/{user_id}/employee-code")
def update_employee_code(user_id: str, employee_code: str, db: Session = Depends(get_db)):
    try:
        user_uuid = uuid.UUID(user_id)
    except ValueError:
        raise HTTPException(status_code=400, detail="ID utilisateur invalide")
    
    # Vérifier que l'employé existe
    employe = db.query(Employe).filter(Employe.id == user_uuid).first()
    if not employe:
        raise HTTPException(status_code=404, detail="Utilisateur non trouvé")
    
    # Vérifier que le nouveau code n'est pas déjà utilisé
    existing_employe = db.query(Employe).filter(Employe.employee_code == employee_code).first()
    if existing_employe and existing_employe.id != user_uuid:
        raise HTTPException(status_code=400, detail="Ce code employé est déjà utilisé")
    
    # Mettre à jour le code
    employe.employee_code = employee_code
    db.commit()
    
    return {"message": f"Code employé mis à jour avec succès: {employee_code}"}

# Route pour signaler la sortie (mettre à jour heure_depart)
@router.put("/presences/{presence_id}/sortie")
def signal_sortie(presence_id: str, db: Session = Depends(get_db)):
    try:
        presence_uuid = uuid.UUID(presence_id)
    except ValueError:
        raise HTTPException(status_code=400, detail="ID de présence invalide")
    
    presence = db.query(Presence).filter(Presence.id == presence_uuid).first()
    if not presence:
        raise HTTPException(status_code=404, detail="Présence non trouvée")
    
    if presence.heure_depart:
        raise HTTPException(status_code=400, detail="La sortie a déjà été signalée")
    
    # Mettre à jour l'heure de sortie
    presence.heure_depart = datetime.now()
    presence.type_presence = "sortie"
    
    db.commit()
    
    return {"message": "Sortie signalée avec succès", "heure_depart": presence.heure_depart}

# Route pour modifier le type de présence
@router.put("/presences/{presence_id}/type")
def modifier_type_presence(presence_id: str, type_presence: str, db: Session = Depends(get_db)):
    try:
        presence_uuid = uuid.UUID(presence_id)
    except ValueError:
        raise HTTPException(status_code=400, detail="ID de présence invalide")
    
    presence = db.query(Presence).filter(Presence.id == presence_uuid).first()
    if not presence:
        raise HTTPException(status_code=404, detail="Présence non trouvée")
    
    # Vérifier que le type est valide
    types_valides = ["entrée", "sortie", "pause", "reprise"]
    if type_presence not in types_valides:
        raise HTTPException(status_code=400, detail=f"Type de présence invalide. Types autorisés: {types_valides}")
    
    presence.type_presence = type_presence
    db.commit()
    
    return {"message": f"Type de présence mis à jour: {type_presence}"}

# Route pour ajouter des notes à une présence
@router.put("/presences/{presence_id}/notes")
def ajouter_notes_presence(presence_id: str, notes: str, db: Session = Depends(get_db)):
    try:
        presence_uuid = uuid.UUID(presence_id)
    except ValueError:
        raise HTTPException(status_code=400, detail="ID de présence invalide")
    
    presence = db.query(Presence).filter(Presence.id == presence_uuid).first()
    if not presence:
        raise HTTPException(status_code=404, detail="Présence non trouvée")
    
    presence.notes = notes
    db.commit()
    
    return {"message": "Notes ajoutées avec succès"}
