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
    a->error("LocateHandleBuffer: 0x%016llX\n", (UINT64)Status);
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
  
  if (!eacc) {
    return;
  }

  if(eacc->HandleBuffer != NULL) {
    gBS->FreePool(eacc->HandleBuffer);
  }

  pci_mfree(eacc);
}

static int ecam_read(struct pci_dev *d, int pos, byte *buf, int len)
{
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;

    if (pos >= 4096) {
      d->access->error("pos >= 4096\n");
      return 0;
    }

    if (buf == NULL) {
      d->access->error("buf == NULL\n");
      return 0;
    }

    PciIo = d->backend_data;
    if(PciIo == NULL) {
      d->access->error("PciIo == NULL\n");
      return 0;
    }

    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, pos, len, buf);
    if (EFI_ERROR (Status)) {
      d->access->error("PciIo Read Error: 0x%016llX\n", (UINT64)Status);
      return 0;
    }

    return 1;
}

static int ecam_write(struct pci_dev *d, int pos, byte *buf, int len)
{
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;

    if (pos >= 4096) {
      d->access->error("pos >= 4096\n");
      return 0;
    }

    if (buf == NULL) {
      d->access->error("buf == NULL\n");
      return 0;
    }

    PciIo = d->backend_data;
    if(PciIo == NULL) {
      d->access->error("PciIo == NULL\n");
      return 0;
    }

    Status = PciIo->Pci.Write(PciIo, EfiPciIoWidthUint8, pos, len, buf);
    if (EFI_ERROR (Status)) {
      d->access->error("PciIo Write Error: 0x%016llX\n", (UINT64)Status);
      return 0;
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
        d->vendor_id = VID;
        ecam_read(d, PCI_DEVICE_ID, (UINT8*)&d->device_id, 2);
        ecam_read(d, PCI_HEADER_TYPE, (UINT8*)&d->hdrtype, 1);
        d->hdrtype &= 0x7F;
        pci_link_dev(a, d);
      } else {
        a->debug("Skipping %04llX:%02llX:%02llX.%lld - Invalid Vendor ID: 0x%04X\n", (UINT64)uefiDomain, (UINT64)uefiBus, (UINT64)uefiDev, (UINT64)uefiFunc, VID);
      }
    } else {
      a->error("OpenProtocol: 0x%016llX\n", (UINT64)Status);
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
