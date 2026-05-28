#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PciIo.h>
#include "internal.h"

// Back-end data linked to struct pci_access
struct uefi_ecam_access {
    UINTN HandleCount;
    EFI_HANDLE *HandleBuffer;
};

static void ecam_config(struct pci_access *a)
{
  return;
}

static int ecam_detect(struct pci_access *a)
{
  EFI_STATUS Status;
  struct uefi_ecam_access *eacc;

  eacc = pci_malloc(a, sizeof(struct uefi_ecam_access));
  if(eacc == NULL)
  {
    a->error("malloc failure");
    return 0;
  }
  eacc->HandleBuffer = NULL;
  eacc->HandleCount = 0;

  // 2. Fetch all physical PCI device handles
  Status = gBS->LocateHandleBuffer(
    ByProtocol,
    &gEfiPciIoProtocolGuid,
    NULL,
    &eacc->HandleCount,
    &eacc->HandleBuffer
  );
  if (EFI_ERROR(Status))
  {
    a->error("LocateHandleBuffer");
    pci_mfree(eacc);
    a->backend_data = NULL;
    return 0;
  }
  a->backend_data = eacc;
  a->debug("HandleCount = %lld\n", eacc->HandleCount);
  return 1;
}

static void ecam_init(struct pci_access *a)
{
  return;
}

static void ecam_cleanup(struct pci_access *a)
{
  struct uefi_ecam_access *eacc = a->backend_data;
  if(eacc->HandleBuffer != NULL) {
    gBS->FreePool(eacc->HandleBuffer);
  }
  if(eacc != NULL) {
    pci_mfree(eacc);
  }
}

static UINT8 ecam_read8(struct pci_dev *d, INT32 offset)
{
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;
    UINT8 Data = 0xFF;

    PciIo = d->backend_data;
    if(PciIo != NULL) {
      Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, offset, 1, &Data);
      if (EFI_ERROR (Status)) {
        d->access->error("PciIo Read Error: 0x%016llX\n", Status);
      }
    }

    return Data;
}

static UINT16 ecam_read16(struct pci_dev *d, INT32 offset)
{
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;
    UINT16 Data = 0xFFFF;

    PciIo = d->backend_data;
    if(PciIo != NULL) {
      Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, offset, 1, &Data);
      if (EFI_ERROR (Status)) {
        d->access->error("PciIo Read Error: 0x%016llX\n", Status);
      }
    }

    return Data;
}

static UINT32 ecam_read32(struct pci_dev *d, INT32 offset)
{
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;
    UINT32 Data = 0xFFFFFFFF;

    PciIo = d->backend_data;
    if(PciIo != NULL) {
      Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, offset, 1, &Data);
      if (EFI_ERROR (Status)) {
        d->access->error("PciIo Read Error: 0x%016llX\n", Status);
      }
    }

    return Data;
}

static int ecam_read(struct pci_dev *d, int pos, byte *buf, int len)
{
  if (pos >= 4096)
    return 0;

  if (len != 1 && len != 2 && len != 4)
    return pci_generic_block_read(d, pos, buf, len);

  switch (len)
    {
    case 1:
      buf[0] = ecam_read8(d, pos);
      break;
    case 2:
      ((u16 *) buf)[0] = ecam_read16(d, pos);
      break;
    case 4:
      ((u32 *) buf)[0] = ecam_read32(d, pos);
      break;
    }

    return 1;
}

static void ecam_write8(struct pci_dev *d, INT32 offset, UINT8 data)
{
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;

    PciIo = d->backend_data;
    if(PciIo != NULL) {
      Status = PciIo->Pci.Write(PciIo, EfiPciIoWidthUint8, offset, 1, &data);
      if (EFI_ERROR (Status)) {
        d->access->error("PciIo Write Error: 0x%016llX\n", Status);
      }
    }
}

static void ecam_write16(struct pci_dev *d, INT32 offset, UINT16 data)
{
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;

    PciIo = d->backend_data;
    if(PciIo != NULL) {
      Status = PciIo->Pci.Write(PciIo, EfiPciIoWidthUint16, offset, 1, &data);
      if (EFI_ERROR (Status)) {
        d->access->error("PciIo Write Error: 0x%016llX\n", Status);
      }
    }
}

static void ecam_write32(struct pci_dev *d, INT32 offset, UINT32 data)
{
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;

    PciIo = d->backend_data;
    if(PciIo != NULL) {
      Status = PciIo->Pci.Write(PciIo, EfiPciIoWidthUint32, offset, 1, &data);
      if (EFI_ERROR (Status)) {
        d->access->error("PciIo Write Error: 0x%016llX\n", Status);
      }
    }
}

static int ecam_write(struct pci_dev *d, int pos, byte *buf, int len)
{
  if (pos >= 4096)
    return 0;

  if (len != 1 && len != 2 && len != 4)
    return pci_generic_block_write(d, pos, buf, len);

  switch (len)
    {
    case 1:
      ecam_write8(d, pos, buf[0]);
      break;
    case 2:
      ecam_write16(d, pos, ((u16 *) buf)[0]);
      break;
    case 4:
      ecam_write32(d, pos, ((u32 *) buf)[0]);
      break;
    }

  return 1;
}

static void ecam_scan(struct pci_access *a)
{
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo;  
  struct uefi_ecam_access *eacc = a->backend_data;
  struct pci_dev *d;
  UINT16 VID;

  for (UINTN i = 0; i < eacc->HandleCount; i++) {
    Status = gBS->OpenProtocol(
                    eacc->HandleBuffer[i],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );  
    if (!EFI_ERROR(Status)) {
      UINTN uefiDomain;
      UINTN uefiBus;
      UINTN uefiDev;
      UINTN uefiFunc;

      PciIo->GetLocation(PciIo, &uefiDomain, &uefiBus, &uefiDev, &uefiFunc);
      PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_VENDOR_ID, 1, &VID);
      if(VID != 0xFFFF) {
        d = pci_alloc_dev(a);
        d->domain = uefiDomain;
        d->bus = uefiBus;
        d->dev = uefiDev;
        d->func = uefiFunc;
        d->known_fields = PCI_FILL_IDENT;
        d->backend_data = PciIo;
        d->vendor_id = ecam_read16(d, PCI_VENDOR_ID);
        d->device_id = ecam_read16(d, PCI_DEVICE_ID);
        d->hdrtype = ecam_read8(d, PCI_HEADER_TYPE) & 0x7F;
        pci_link_dev(a, d);
      } else {
        a->debug("Skipping %04llX:%02llX:%02llX.%lld - Invalid Vender ID: 0x%04X\n", uefiDomain, uefiBus, uefiDev, uefiFunc, VID);
      }
    } else {
      a->error("OpenProtocol: 0x%016llX\n", Status);
    }
  }
}

struct pci_methods pm_ecam = {
  .name = "ecam",
  .help = "Raw memory mapped access using PCIe ECAM interface",
  .config = ecam_config,
  .detect = ecam_detect,
  .init = ecam_init,
  .cleanup = ecam_cleanup,
  .scan = ecam_scan,
  .fill_info = pci_generic_fill_info,
  .read = ecam_read,
  .write = ecam_write,
};
