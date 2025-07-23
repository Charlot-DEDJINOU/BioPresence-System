from pydantic import BaseModel, EmailStr
from typing import Optional, List
from datetime import date, datetime
import uuid

# class EmployeBase(BaseModel):
#     full_name: str
#     email: Optional[EmailStr] = None
#     telephone: Optional[str] = None
#     poste: Optional[str] = None
#     date_recrutement: date
#     salaire: Optional[float] = None
#     empreinte_1: str
#     empreinte_2: str
#     empreinte_3: str

class EmployeBase(BaseModel):
    full_name: str
    email: str
    phone: Optional[str]
    date_of_birth: Optional[date]
    address: Optional[str]
    gender: Optional[str]
    marital_status: Optional[str]

    employee_code: Optional[str] = None
    position: Optional[str]
    department: Optional[str]
    hire_date: Optional[date]
    contract_type: Optional[str]
    salary: Optional[float]
    contract_end_date: Optional[date]
    work_start_time: Optional[str]
    work_end_time: Optional[str]

    education_level: Optional[str]
    experience: Optional[str]
    skip_biometric: Optional[bool]

    class Config:
        from_attributes = True





class FingerprintVerificationData(BaseModel):
    code_unique: str
    fingerprint: str  # base64


class EmployeCreate(EmployeBase):
    pass

class EmployeRead(EmployeBase):
    id: uuid.UUID
    company_id: str
    code_unique: str
    actif: bool
    created_at: datetime

    class Config:
        orm_mode = True

# Nouveaux schémas pour Empreinte
class EmpreinteBase(BaseModel):
    empreinte_id: int

class EmpreinteCreate(EmpreinteBase):
    pass

class EmpreinteRead(EmpreinteBase):
    id: uuid.UUID
    employe_id: uuid.UUID
    created_at: datetime

    class Config:
        orm_mode = True

# Nouveaux schémas pour Presence
class PresenceBase(BaseModel):
    employee_code: str
    date_presence: Optional[date] = None
    heure_arrivee: Optional[datetime] = None
    heure_depart: Optional[datetime] = None
    type_presence: Optional[str] = "entrée"
    notes: Optional[str] = None

class PresenceCreate(BaseModel):
    employee_code: str

class PresenceRead(PresenceBase):
    id: uuid.UUID
    employe_id: uuid.UUID
    created_at: datetime

    class Config:
        orm_mode = True

# Schémas pour les routes d'empreintes
class EmpreintesUpdate(BaseModel):
    empreinte_ids: List[int]

class EmpreintesResponse(BaseModel):
    empreinte_ids: List[int]